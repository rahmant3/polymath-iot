/*
 * hardware_defs.c
 *
 *  Created on: May 9, 2021
 *      Author: Hemant Gopalsing
 */
#include "main.h"
#include "hardware_defs.h"


I2C_HandleTypeDef * I2C_arrayss[num_i2c_interfaces] =
{
		[HW_I2C1] = &hi2c1,
};
