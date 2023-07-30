#include "i2c_check.h"
#include <stdio.h>
#include "serial.h"


void i2c_list_bus(I2C_HandleTypeDef *hi2c)
{
	int i;
	int ret;
	char buffer[20];

	for (i = 1; i < 0x7f; i++) {
		buffer[1] = 0;

//		ret = HAL_I2C_Master_Transmit(hi2c, i*2, (unsigned char *)buffer, 1, 100);
		ret = HAL_I2C_Master_Receive(hi2c, i*2, (unsigned char *)buffer, 1, 10);
		if (ret == 0) {
			sprintf(buffer,"I2C slave at 0x%02x\n", i);
			serial_send_str(buffer);
		}
	}
}

