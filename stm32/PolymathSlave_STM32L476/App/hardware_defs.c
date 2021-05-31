/*
 * hardware_defs.c
 *
 *  Created on: May 9, 2021
 *      Author: Hemant Gopalsing
 */
#include "main.h"
#include "hardware_defs.h"


I2C_HandleTypeDef * i2c_interfaces[HW_NUM_I2C] =
{
		[HW_I2C1] = &hi2c1,
		[HW_I2C2] = &hi2c2,
};
