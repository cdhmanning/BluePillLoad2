/*
 * The way the sampling works is as follows:
 * The Systick triggers the ADC control.
 * The ADC control then uses ADC interrupts to issue 16 samples on each ADC channel
 * (interleaved). As each sample arrives, it is added to a sum.
 * The result is a sum of 16 samples for each ADC.
 */

#include "adc_control.h"
#include "main.h"

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "debug_pin.h"


#define SAMPLES_PER_SUM	16

struct {
	uint32_t sum[2];
	uint32_t n_cycles;
	uint32_t complete;
	uint32_t busy;
}adc_ctl;

void adc_ctl_init(void)
{

}

void adc_ctl_trigger(void)
{
	if (adc_ctl.busy)
		return;

	adc_ctl.sum[0] = 0;
	adc_ctl.sum[1] = 0;
	adc_ctl.complete = 0;
	adc_ctl.busy = 1;
	adc_ctl.n_cycles = SAMPLES_PER_SUM * 2;

	ADC1->SR &= ~ADC_SR_EOC;
	ADC2->SR &= ~ADC_SR_EOC;
    ADC1->CR2 |= ADC_CR2_SWSTART;

}

int adc_ctl_get_values(uint32_t *values, int n_values)
{
	int i;

	if (!adc_ctl.complete) {
		return -1;
	}

	if (n_values > 2)
		n_values = 2;

	for (i=0; i < n_values; i++)
		values[i] = adc_ctl.sum[i];

	return n_values;
}

#define is_end_of_conversion(adcptr) (((adcptr)->SR) & ADC_SR_EOC)
void adc_ctl_isr(void)
{
	uint32_t this_adc;
	uint32_t val;

	debug_1(1);

	if (is_end_of_conversion(ADC1)) {
		this_adc = 0;
		val = ADC1->DR;
	} else if (is_end_of_conversion(ADC2)) {
		this_adc = 1;
		val = ADC1->DR;

	} else {
		while(1) {};
	}

	adc_ctl.sum[this_adc] += val;

	ADC1->SR &= ~ADC_SR_EOC;
	ADC2->SR &= ~ADC_SR_EOC;

	adc_ctl.n_cycles--;

	if(adc_ctl.n_cycles > 0) {
		/* Start the other ADC */
		if (this_adc == 0)
			ADC2->CR2 |= ADC_CR2_SWSTART;
		else
			ADC2->CR2 |= ADC_CR2_SWSTART;
	} else {
		adc_ctl.complete = 1;
		adc_ctl.busy = 0;
	}

	debug_1(0);
}
