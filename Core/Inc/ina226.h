
#ifndef __INC_INA226_H__
#define __INC_INA226_H__

#include <stdint.h>

struct ina226_ctl
{
	uint32_t i2c_addr;
	int16_t bus_reg;
	int16_t shunt_reg;
	int		 bus_mV;
	int		 shunt_uV;
	int 	shunt_mA;
};

int ina226_init(struct ina226_ctl *ctl, uint32_t i2c_addr);
int ina226_update(struct ina226_ctl *ctl);

#endif
