/*
 * This code originally from github.com/eziya/STM32_HAL_I2C_HD44780
 * However it has been modified a bit...
 */

#include "i2c_lcd.h"

static void SendCommand(struct i2c_lcd *lcd, uint8_t);
static void SendChar(struct i2c_lcd *lcd, uint8_t);
static void Send(struct i2c_lcd *lcd, uint8_t, uint8_t);
static void Write4Bits(struct i2c_lcd *lcd, uint8_t);
static void ExpanderWrite(struct i2c_lcd *lcd, uint8_t);
static void PulseEnable(struct i2c_lcd *lcd, uint8_t);
static void DelayInit(void);
static void DelayUS(uint32_t);

static uint8_t special0[8] = {
        0b00000,
        0b11001,
        0b11011,
        0b00110,
        0b01100,
        0b11011,
        0b10011,
        0b00000
};

static uint8_t special1[8] = {
        0b11000,
        0b11000,
        0b00110,
        0b01001,
        0b01000,
        0b01001,
        0b00110,
        0b00000
};

struct special_character_def {
	uint8_t character_id;
	uint8_t *bitmap;
};

struct special_character_def special_list[] = {
		{.character_id = 0, .bitmap = special0},
		{.character_id = 1, .bitmap = special1},
};


static void create_special_characters(struct i2c_lcd *lcd)
{
	int i;

	for (i = 0; i < (sizeof(special_list)/sizeof(special_list[0])); i++)
		HD44780_CreateSpecialChar(lcd, special_list[i].character_id, special_list[i].bitmap);
}

void HD44780_Init(struct i2c_lcd *lcd, I2C_HandleTypeDef *bus, uint8_t addr_7bits, uint8_t rows)
{
  lcd->hi2c = bus;
  lcd->i2c_addr = (addr_7bits << 1);

  lcd->rows = rows;

  lcd->backlight = LCD_NOBACKLIGHT;

  lcd->function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

  if (lcd->rows > 1)
  {
    lcd->function |= LCD_2LINE;
  }
  else
  {
    lcd->function |= LCD_5x10DOTS;
  }

  /* Wait for initialization */
  DelayInit();
  //HAL_Delay(50); LCD must only be initialised 50msec after power up.

  ExpanderWrite(lcd, lcd->backlight);
  HAL_Delay(1);

  /* 4bit Mode.  3x 0x03, the 0x02*/
  Write4Bits(lcd, 0x03 << 4); DelayUS(4500);
  Write4Bits(lcd, 0x03 << 4); DelayUS(4500);
  Write4Bits(lcd, 0x03 << 4); DelayUS(4500);
  Write4Bits(lcd, 0x02 << 4); DelayUS(100);

  /* Display Control */
  SendCommand(lcd, LCD_FUNCTIONSET | lcd->function);

  lcd->control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  HD44780_SetDisplayVisible(lcd, 1);
  HD44780_Clear(lcd);

  /* Display Mode */
  lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
  DelayUS(4500);

  create_special_characters(lcd);

  HD44780_Home(lcd);
}

void HD44780_Clear(struct i2c_lcd *lcd)
{
  SendCommand(lcd, LCD_CLEARDISPLAY);
  DelayUS(2000);
}

void HD44780_Home(struct i2c_lcd *lcd)
{
  SendCommand(lcd, LCD_RETURNHOME);
  DelayUS(2000);
}

void HD44780_SetCursor(struct i2c_lcd *lcd, uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= lcd->rows)
    row = lcd->rows-1;
  SendCommand(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

static void HD44780_SetControl(struct i2c_lcd *lcd, uint8_t ctl_bit, int val)
{
	if (val)
		lcd->control |= ctl_bit;
	else
		lcd->control &= ~ctl_bit;

	SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->control);

}

void HD44780_SetDisplayVisible(struct i2c_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_DISPLAYON, val);
}

void HD44780_SetCursorVisible(struct i2c_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_CURSORON, val);
}

void HD44780_SetBlink(struct i2c_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_BLINKON, val);
}

void HD44780_ScrollDisplayLeft(struct i2c_lcd *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void HD44780_ScrollDisplayRight(struct i2c_lcd *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void HD44780_LeftToRight(struct i2c_lcd *lcd)
{
  lcd->mode |= LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_RightToLeft(struct i2c_lcd *lcd)
{
  lcd->mode &= ~LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_AutoScroll(struct i2c_lcd *lcd)
{
  lcd->mode |= LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_NoAutoScroll(struct i2c_lcd *lcd)
{
  lcd->mode &= ~LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_CreateSpecialChar(struct i2c_lcd *lcd, uint8_t location, uint8_t charmap[])
{
  location &= 0x7;
  SendCommand(lcd, LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++)
  {
    SendChar(lcd, charmap[i]);
  }
}

void HD44780_PrintSpecialChar(struct i2c_lcd *lcd, uint8_t index)
{
  SendChar(lcd, index);
}

void HD44780_LoadCustomCharacter(struct i2c_lcd *lcd, uint8_t char_num, uint8_t *rows)
{
  HD44780_CreateSpecialChar(lcd, char_num, rows);
}

void HD44780_PrintStr(struct i2c_lcd *lcd, const char c[])
{
  while(*c) {
	  SendChar(lcd, *c);
	  c++;
  }
}

void HD44780_SetBacklight(struct i2c_lcd *lcd, uint8_t new_val)
{
  lcd->backlight=new_val ? LCD_BACKLIGHT : LCD_NOBACKLIGHT;
  ExpanderWrite(lcd, 0);
}

static void SendCommand(struct i2c_lcd *lcd, uint8_t cmd)
{
  Send(lcd, cmd, 0);
}

static void SendChar(struct i2c_lcd *lcd, uint8_t ch)
{
  Send(lcd, ch, RS);
}

static void Send(struct i2c_lcd *lcd, uint8_t value, uint8_t mode)
{
  uint8_t highnib = value & 0xF0;
  uint8_t lownib = (value<<4) & 0xF0;
  Write4Bits(lcd, (highnib)|mode);
  Write4Bits(lcd, (lownib)|mode);
}

static void Write4Bits(struct i2c_lcd *lcd, uint8_t value)
{
  //ExpanderWrite(lcd, value);
  PulseEnable(lcd, value);
}

static void ExpanderWrite(struct i2c_lcd *lcd, uint8_t _data)
{
  uint8_t data = _data | lcd->backlight;
  HAL_I2C_Master_Transmit(lcd->hi2c, lcd->i2c_addr, (uint8_t*)&data, 1, 10);
}

static void PulseEnable(struct i2c_lcd *lcd, uint8_t _data)
{
  ExpanderWrite(lcd, _data | ENABLE);
  //DelayUS(20);

  ExpanderWrite(lcd, _data & ~ENABLE);
  //DelayUS(20);
}

static void DelayInit(void)
{
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
  CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

  DWT->CYCCNT = 0;

  /* 3 NO OPERATION instructions */
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
}

static void DelayUS(uint32_t us) {
  uint32_t cycles = (SystemCoreClock/1000000L)*us;
  uint32_t start = DWT->CYCCNT;
  volatile uint32_t cnt;

  do
  {
    cnt = DWT->CYCCNT - start;
  } while(cnt < cycles);
}
