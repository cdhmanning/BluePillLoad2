
#ifndef __INC_ADC_CONTROL_H__
#define __INC_ADC_CONTROL_H__
#include <stdint.h>

void adc_ctl_init(void);
void adc_ctl_trigger(void);
int adc_ctl_get_values(uint32_t *values, int n_values);
void adc_ctl_isr(void);

#endif /* INC_ADC_CONTROL_H_ */
