/*
 * mics_sensor.h
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

#ifndef HARDWARE_DEFS_H_
#define HARDWARE_DEFS_H_

#include "stm32f4xx_hal.h"
#include "../sensor_interface.h"
#include "main.h"
#include <stdbool.h>

typedef enum
{
	HW_I2C1,

	num_i2c_interfaces,
}interface_selection_t;


extern I2C_HandleTypeDef * I2C_arrayss[num_i2c_interfaces];

#endif
