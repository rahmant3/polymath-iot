/*
 * mics_sensor.h
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

#ifndef MICS_MICS_SENSOR_H_
#define MICS_MICS_SENSOR_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "../sensor_interface.h"
#include "main.h"

#define MICS_PRIAMRY_I2C_ADDRESS 			0x70
#define MICS_VZ_89TE_ADDR_CMD_GETSTATUS		0x0C
#define MICS_VZ_89TE_DATE_CODE 				0x0D

typedef enum
{
	MICS_I2C2,

	mics_num_interfaces,
}mics_interface_to_use;


//unsigned int getSensorReading (uint8_t * rx_buffer);
uint8_t getSensorReading (mics_sensor_command_config_t * mics_command_config, interface_selection_t interface);

#endif /* MICS_MICS_SENSOR_H_ */
