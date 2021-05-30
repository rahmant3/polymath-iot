/*
 * mics_sensor.h
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

#ifndef MICS_MICS_SENSOR_H_
#define MICS_MICS_SENSOR_H_

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdint.h>

#include  "sensor_interface.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define MICS_COMMAND_FAIL                   	0
#define MICS_COMMAND_SUCESS                 	1
#define MICS_PRIAMRY_I2C_ADDRESS 				0x70

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------

typedef enum
{
	MICS_VZ_89TE_CMD_GET_SENSOR_VALS = 0x0C,
	MICS_VZ_89TE_CMD_GET_DATE_CODE   = 0x0D,
	MICS_VZ_89TE_CMD_GET_R0_VAL      = 0x16,
	MICS_VZ_89TE_CMD_SET_PPM_CO2     = 0x08,

}mics_commands_t;

typedef struct 
{
	uint8_t voc_value;
	uint8_t co2_level_value;
	uint8_t raw_val_msb;
	uint8_t raw_value;
	uint8_t raw_val_lsb;
	uint8_t error_value;
	uint8_t crc_val;

}mics_sensor_data_t;

typedef int (*MICSTxFcnPtr) (uint8_t dev_id, mics_commands_t command, uint8_t * data);
typedef int (*MICSRxFcnPtr) (uint8_t dev_id, mics_commands_t command, uint8_t * data);

// --------------------------------------------------------------------------------------------------------------------
// CONSTANT
// --------------------------------------------------------------------------------------------------------------------

//! Driver to get a CO2 reading from the sensor (ppm x100). No parameters required.
extern const sensorDriver_t sensorDriverCO2_micsvz89te;

//! Driver to get a TVOC reading from the sensor (ppm x100). No parameters required.
extern const sensorDriver_t sensorDriverVOC_micsvz89te;

#endif /* MICS_MICS_SENSOR_H_ */
