#include "ina226.h"

#include "main.h"

#define INA226_REG_CONFIG	0x00
#define INA226_REG_SHUNT	0x01
#define INA226_REG_BUS		0x02

static int ina226_read_reg(struct ina226_ctl *ctl, uint8_t reg, uint16_t *val)
{
	uint8_t buffer[2];
	int ret;

	buffer[0] = reg;
	ret = HAL_I2C_Master_Transmit(&hi2c1, ctl->i2c_addr, buffer, 1, 1000);
	if (ret != HAL_OK)
		return ret;
	ret = HAL_I2C_Master_Receive(&hi2c1, ctl->i2c_addr, buffer, 2, 1000);

	if (ret == HAL_OK) {
		*val = (((uint16_t)buffer[0]) << 8) | buffer[1];
	}
	return ret;
}



static int ina226_write_reg(struct ina226_ctl *ctl, uint8_t reg, uint16_t val)
{
	uint8_t buffer[3];
	int ret;

	buffer[0] = reg;
	buffer[1] = val >> 8;
	buffer[2] = val;

	ret = HAL_I2C_Master_Transmit(&hi2c1, ctl->i2c_addr, buffer, 3, 1000);
	if (ret != HAL_OK)
		return ret;
	return ret;
}


int ina226_update(struct ina226_ctl *ctl)
{
	int ret;

	ret = ina226_read_reg(ctl, INA226_REG_BUS, (uint16_t *)&ctl->bus_reg);
	ret = ina226_read_reg(ctl, INA226_REG_SHUNT, (uint16_t *)&ctl->shunt_reg);

	ctl->bus_mV = ctl->bus_reg + ctl->bus_reg/4;	/* x 1.25mV */
	ctl->shunt_uV = ctl->shunt_reg * 2 + ctl->shunt_reg/2;	/* x 2.5uV */
	ctl->shunt_mA = ctl->shunt_uV /100;

	return ret;
}

int ina226_init(struct ina226_ctl *ctl, uint32_t i2c_addr)
{
	int ret;
	uint16_t val;

	ctl->i2c_addr = i2c_addr << 1;

	ret = ina226_read_reg(ctl, 0xFE, &val);
	ret = ina226_read_reg(ctl, 0xFF, &val);

	ret = ina226_write_reg(ctl, INA226_REG_CONFIG, 0x4097);

	HAL_Delay(2);

	ret = ina226_update(ctl);

	return ret;
}

