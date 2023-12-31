#include "load.h"
#include "main.h"
#include "serial.h"
#include "ina226.h"
#include "debug_pin.h"
#include "i2c_lcd.h"

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <string.h>


typedef enum {
	LoadModeCurrent = 0,
	LoadModePower,
	LoadModeVoltage,
	LoadModePWM
} LoadMode;


static struct {
	uint32_t load_on;
	int setpt_pwm;
	int setpt_mA;
	int setpt_mV;
	int setpt_mW;
	LoadMode vc_index;
	const char *mode_str;

	uint32_t over_voltage;
	uint32_t under_voltage;
	uint32_t last_pwm;
} control_values;

static struct {
	uint32_t V_adc;	/* 16-bit */
	uint32_t I_adc; /* 16-bit */

	/*
	 * Values calculated from ADC.
	 */
	uint32_t mV;
	uint32_t mA;
	uint32_t mW;
}control_inputs;

#define PWM_MIN			0
#define PWM_MAX 		1000
#define PWM_RANGE		(PWM_MAX - PWM_MIN)
#define PWM_CONTROL_INC	(PWM_RANGE/20)
#define PWM_SMALL_INC	1
#define PWM_LARGE_INC	(PWM_RANGE/50)

#define PWM_UI_INC		(PWM_RANGE/100)

#define mA_MIN			0
#define mA_MAX 			1000
#define mA_CONTROL_INC	50
#define mA_UI_INC		50

#define mV_MIN			0
#define mV_MAX 			20000
#define mV_CONTROL_INC	100
#define mV_UI_INC		50

#define mV_OVER_VOLTAGE (mV_MAX + 1000)

#define mW_MIN			0
#define mW_MAX 			10000
#define mW_CONTROL_INC	100
#define mW_UI_INC		100

#define uW_MAX			(mW_MAX * 1000)


extern struct ina226_ctl ina226;

struct value_controller {
	const char *name;
	int *value;
	int min_val;
	int max_val;
	int increment;
};

static const struct value_controller vc_list[] = {
	[LoadModeCurrent] = { "Current", &control_values.setpt_mA,  mA_MIN,  mA_MAX,  mA_UI_INC},
	[LoadModePower]   =	{ "Power  ", &control_values.setpt_mW,  mW_MIN,  mW_MAX,  mW_UI_INC},
	[LoadModeVoltage] =	{ "Voltage", &control_values.setpt_mV,  mV_MIN,  mV_MAX,  mV_UI_INC},
	[LoadModePWM]     =	{ "PWM    ", &control_values.setpt_pwm, PWM_MIN, PWM_MAX, PWM_UI_INC},
};


#define N_VC	(sizeof(vc_list)/ sizeof(vc_list[0]))

#define pwm_mode() (control_values.vc_index == LoadModePWM)
#define current_mode() (control_values.vc_index == LoadModeCurrent)
#define power_mode() (control_values.vc_index == LoadModePower)

void load_pwm_set(uint32_t val)
{
	TIM2->CCR1 = val;

	if (control_values.last_pwm != val) {
		HAL_GPIO_WritePin(INC_DEC_LED, (control_values.last_pwm < val) ? 0 : 1);
		control_values.last_pwm = val;
	}
}


static void vc_inc_dec(uint32_t do_inc)
{
	const struct value_controller *vc;
	int *val;

	if (control_values.vc_index >= N_VC)
		return;

	vc = &vc_list[control_values.vc_index];
	val = vc->value;

	if (do_inc)
		*val+= vc->increment;
	else
		*val-= vc->increment;

	if(*val < vc->min_val)
		*val = vc->min_val;
	if(*val > vc->max_val)
		*val = vc->max_val;
}

static void select_value(uint32_t sel)
{
	if (sel >= N_VC)
		return;

	control_values.vc_index = sel;
	control_values.mode_str = vc_list[sel].name;
}

static void vc_increment(void)
{
	uint32_t vc = control_values.vc_index;

	vc++;
	if (vc >= N_VC)
		vc = 0;
	select_value(vc);
}

void load_rotation_isr(uint16_t GPIOPin)
{
	vc_inc_dec(HAL_GPIO_ReadPin(ROTA0));
}

struct button {
	uint32_t debounced;
	uint32_t pending;
	uint32_t count;
	void (*event_fn)(uint32_t val);
};

struct button rotary_push;
struct button on_off_button;

void button_init(struct button *b, uint32_t val, void (*event_fn)(uint32_t val))
{
	val = !!val;
	b->pending = val;
	b->debounced = val;
	b->event_fn = event_fn;
}

void button_update(struct button *b, uint32_t val)
{
	val = !!val;
	if (b->pending != val) {
		b->pending = val;
		b->count = 20; /* settling time */
	} else if (b->count > 0) {
		b->count--;
	} else if (b->debounced != val){
		/* Settled */
		b->debounced = val;
		if (b->event_fn)
			b->event_fn(val);
	}
}

uint32_t button_get_debounced(struct button *b)
{
	return b->debounced;
}

void load_capture_adc_values(void)
{
	control_inputs.V_adc = ADC1->DR & 0x0fff;
	control_inputs.I_adc = ADC2->DR & 0x0fff;
}



static void load_check_voltage(void)
{
	control_values.under_voltage = (control_inputs.mV < 500);
	control_values.over_voltage = (control_inputs.mV > mV_OVER_VOLTAGE);

	if (control_values.over_voltage)
		control_values.load_on = 0;
}

static uint32_t load_calculate_pwm(void)
{
	int target_mA = 0;
	int mA_limit;
	int mA_error;
	int pwm_inc;
	int new_pwm;

	/*
	 * Need at least the minimum to prevent calculations overflowing.
	 * Just don't conduct if under voltage.
	 */
	if (control_values.under_voltage)
		return 0;

	if (current_mode()) {
		target_mA = control_values.setpt_mA;
	} else {
		/* Power mode.
		 * Calculate a target current from the desired mW
		 * the current measured mV.
		 */
		target_mA = (control_values.setpt_mW * 1000)/control_inputs.mV;
	}

	/*
	 * So far we have the target mA from the set point.
	 * But it might violate the device's max current or power so knock
	 * it down if needed.
	 */

	mA_limit = uW_MAX/control_inputs.mV;
	if (mA_limit > mA_MAX)
		mA_limit = mA_MAX;

	if (target_mA > mA_limit)
		target_mA = mA_limit;

	if (target_mA == 0)
		return 0;
	/*
	 * OK, now we have the real target mA.
	 * Compare that to the current mA and see if we need to increase or
	 * decrease PWM.
	 */

	mA_error = target_mA - (int)(control_inputs.mA);

	/* Now we need to make that into a PWM value. */

	if(mA_error > 100)
		pwm_inc = PWM_LARGE_INC;
	else if (mA_error > 0)
		pwm_inc = PWM_SMALL_INC;
	else if (mA_error == 0)
		pwm_inc = 0;
	else if (mA_error > -100)
		pwm_inc = -PWM_SMALL_INC;
	else /* Smaller than -100 */
		pwm_inc = -PWM_LARGE_INC;

	new_pwm = control_values.last_pwm + pwm_inc;

	if (new_pwm > PWM_MAX)
		new_pwm = PWM_MAX;
	else if (new_pwm < PWM_MIN)
		new_pwm = PWM_MIN;

	return new_pwm;
}

static void load_update(void)
{	uint32_t pwm;

	load_check_voltage();

	if (!control_values.load_on) {
		pwm = 0;
	} else if (pwm_mode()){
		pwm = control_values.setpt_pwm;
	} else {
		/* Control to current or power.
		 */
		pwm = load_calculate_pwm();
	}
	load_pwm_set(pwm);
}

void load_pendsv(void)
{
	debug_2(1);
	load_update();
	debug_2(0);
}

void load_tick0(void)
{
	debug_2(1);
	ina226_read_seq_init(&ina226);
	debug_2(0);
}

void load_tick1(void)
{
	button_update(&rotary_push, HAL_GPIO_ReadPin(ROTAPRESS));
	button_update(&on_off_button, HAL_GPIO_ReadPin(PBUTTONA));

	HAL_GPIO_WritePin(ON_LED, control_values.load_on ? 0 : 1);
}

struct {
	char setpt_mA[10];
	char setpt_mW[10];
	char setpt_mV[10];
	char setpt_pwm[10];
	char setpt_str[10];
	char mode[10];
	char on_off[10];
	char mV[10];
	char mA[10];
	char mW[10];
	char pwm[10];
	char limit[10];
} outstr;

void load_format_data(void)
{
	control_inputs.mA = ina226.shunt_mA;
	control_inputs.mV = ina226.bus_mV;
	control_inputs.mW = (control_inputs.mA * control_inputs.mV)/1000;

	strcpy(outstr.mode, control_values.mode_str);

	sprintf(outstr.setpt_mA,"%5dmA", control_values.setpt_mA);
	sprintf(outstr.setpt_mW,"%5dmW", control_values.setpt_mW);
	sprintf(outstr.setpt_mV,"%5dmV", control_values.setpt_mV);
	sprintf(outstr.setpt_pwm,"%4dPWM", control_values.setpt_pwm);

	switch(control_values.vc_index) {
	case LoadModeCurrent: strcpy(outstr.setpt_str, outstr.setpt_mA); break;
	case LoadModeVoltage: strcpy(outstr.setpt_str, outstr.setpt_mV); break;
	case LoadModePower:   strcpy(outstr.setpt_str, outstr.setpt_mW); break;
	case LoadModePWM:     strcpy(outstr.setpt_str, outstr.setpt_pwm); break;
	default: strcpy(outstr.setpt_str, "aak!"); break;
	}
	strcpy(outstr.on_off, control_values.load_on ? " ON " : "OFF");

	sprintf(outstr.pwm,"PWM%4ld", control_values.last_pwm);
	sprintf(outstr.mA,"%5ldmA", control_inputs.mA);
	sprintf(outstr.mV,"%5lumV", control_inputs.mV);
	sprintf(outstr.mW,"%5lumW", control_inputs.mW);
}

void load_output_serial(void)
{
	static uint32_t n;
	char x[10];

	n++;
	sprintf(x,"%5lu ",n);
	serial_send_str(x);
	serial_send_str("Settings ");
	serial_send_str(outstr.on_off); serial_send_str(" ");
	serial_send_str(outstr.mode); serial_send_str(" ");
	serial_send_str(outstr.setpt_mA); serial_send_str(" ");
	serial_send_str(outstr.setpt_mV); serial_send_str(" ");
	serial_send_str(outstr.setpt_mW); serial_send_str(" ");
	serial_send_str("Readings ");
	serial_send_str(outstr.mA); serial_send_str(" ");
	serial_send_str(outstr.mV); serial_send_str(" ");
	serial_send_str(outstr.mW); serial_send_str("\n");
}

void load_output_lcd(void)
{
	  HD44780_SetCursor(lcd, 0, 0); HD44780_PrintStr(lcd, outstr.mode);

	  HD44780_SetCursor(lcd, 0, 1); HD44780_PrintStr(lcd, outstr.setpt_str);
	  HD44780_SetCursor(lcd, 0, 3); HD44780_PrintStr(lcd, outstr.pwm);

	  HD44780_SetCursor(lcd, 17, 0); HD44780_PrintStr(lcd, outstr.on_off);
	  HD44780_SetCursor(lcd, 13, 1); HD44780_PrintStr(lcd, outstr.mA);
	  HD44780_SetCursor(lcd, 13, 2); HD44780_PrintStr(lcd, outstr.mV);
	  HD44780_SetCursor(lcd, 13, 3); HD44780_PrintStr(lcd, outstr.mW);
}

void load_poll(void)
{
	static uint32_t last_serial = 0;
	static uint32_t last_lcd = 0;
	static uint32_t last_on_off = 0;
	uint32_t do_serial = 0;
	uint32_t do_lcd = 0;
	uint32_t now = get_tick();

	if (now >=  last_serial + 200) {
		last_serial = now;
		do_serial = 1;
	}

	if (!do_serial &&
		(now >=  last_lcd + 2000 ||
		 last_on_off != control_values.load_on) ) {
		last_lcd = now;
		last_on_off = control_values.load_on;
		do_lcd = 1;
	}

	if (do_serial || do_lcd)
		load_format_data();

	if (do_serial)
		load_output_serial();

	if (do_lcd)
		load_output_lcd();
}

void load_rot_pb_event(uint32_t val)
{
	HAL_GPIO_WritePin(ON_LED, val);
	if (val == 0) {
		/* Pressed */
		vc_increment();
	}
}

void load_pbutton_event(uint32_t val)
{
	static uint32_t last_val = 99; /* Daft value */

	if (last_val != val) {
		last_val = val;
		if (val == 0) /*Pressed. */
			control_values.load_on = ! control_values.load_on;
	}
}

void load_init(void)
{
	button_init(&rotary_push, HAL_GPIO_ReadPin(ROTAPRESS), load_rot_pb_event);
	button_init(&on_off_button, HAL_GPIO_ReadPin(PBUTTONA), load_pbutton_event);
	select_value(0);
	load_pwm_set(0);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    serial_send_str("Load started\n");
}


void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		ina226_sm(&ina226);
	}
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		ina226_sm(&ina226);
	}
}


