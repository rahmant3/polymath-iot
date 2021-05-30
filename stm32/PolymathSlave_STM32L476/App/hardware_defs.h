/*
 * mics_sensor.h
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

#ifndef HARDWARE_DEFS_H_
#define HARDWARE_DEFS_H_

#include "main.h"

typedef enum
{
	HW_I2C1,
	HW_I2C2,

	HW_NUM_I2C,
} HW_I2C_t;


extern I2C_HandleTypeDef * i2c_interfaces[HW_NUM_I2C];

#endif
