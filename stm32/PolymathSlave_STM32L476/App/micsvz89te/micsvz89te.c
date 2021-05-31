/*
 * mics_sensor.c
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <string.h>

#include "micsvz89te.h"

#include "hardware_defs.h"
#include "sensor_interface.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define MICS_TX_BUFFER_SIZE		6
#define MICS_RX_BUFFER_SIZE		7
#define I2C_TIMEOUT             100
// --------------------------------------------------------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------
static uint8_t mics_I2C_transmit(uint8_t dev_id, mics_commands_t command, uint8_t * data, I2C_HandleTypeDef * i2c_handle);
static uint8_t mics_I2C_receive(uint8_t dev_id, mics_commands_t command, uint8_t * data, I2C_HandleTypeDef * i2c_handle);
static uint8_t getSensorReading(mics_commands_t command, HW_I2C_t interface);

static int getSensorReading_co2level (int32_t * resultPtr, int slot,sensorParamsPtr params);
static int getSensorReading_voclevel (int32_t * resultPtr, int slot,sensorParamsPtr params);
// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------------------------------------------------------
const sensorDriver_t sensorDriverCO2_micsvz89te =
{
	.getReading = getSensorReading_co2level
};
const sensorDriver_t sensorDriverVOC_micsvz89te =
{
	.getReading = getSensorReading_voclevel
};

// --------------------------------------------------------------------------------------------------------------------
// VARIABLE
// --------------------------------------------------------------------------------------------------------------------
static mics_sensor_data_t local_sensor_data;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION DECLARATION
// --------------------------------------------------------------------------------------------------------------------

/* --------------------------------------------------------------------------------------------------------------------
 * @param
 * 		dev_id  - the I2C device address primary address
 * 		command - command to send to chip
 * 		data    - data to send
 * @ return
 * 		true  - success
 * 		false - fail
 * @ info This function is for transmitting to the MICS
 --------------------------------------------------------------------------------------------------------------------*/
static uint8_t mics_I2C_transmit(uint8_t dev_id, mics_commands_t command, uint8_t * data, I2C_HandleTypeDef * i2c_handle)
{
	uint8_t transmit_address =  (dev_id << 1);

	uint8_t tx_buffer[MICS_TX_BUFFER_SIZE] = {0};

	tx_buffer[0] = command;

	(void)memcpy(&tx_buffer[1], data, MICS_TX_BUFFER_SIZE-2);

	tx_buffer[MICS_TX_BUFFER_SIZE-1] = 0; ////to add CRC check here

	if (HAL_I2C_Master_Transmit(i2c_handle, transmit_address, tx_buffer, MICS_TX_BUFFER_SIZE, I2C_TIMEOUT) == HAL_OK)
	{
		return true;
	}

	return false;
};

/* --------------------------------------------------------------------------------------------------------------------
 * @param
 * 		dev_id  - the I2C device address primary address
 * 		command - command to send to chip
 * 		data    - data to send
 * @ return
 * 		true  - success
 * 		false - fail
 * @ info This function is for transmitting a command to the MICS and receive the data response
 --------------------------------------------------------------------------------------------------------------------*/
static uint8_t mics_I2C_receive(uint8_t dev_id, mics_commands_t command, uint8_t * rx_data, I2C_HandleTypeDef * i2c_handle)
{
	uint8_t tx_buffer[MICS_TX_BUFFER_SIZE] = {0};

	tx_buffer[0] = command;

	tx_buffer[5] = 0; ////to add CRC check here

	uint8_t transmit_address =  (dev_id << 1);

	uint8_t receive_address	 =  (dev_id << 1) | 1;

	if (HAL_I2C_Master_Transmit(i2c_handle, transmit_address, tx_buffer, MICS_TX_BUFFER_SIZE,  I2C_TIMEOUT) == HAL_OK)
	{
		HAL_Delay(100);

		if (HAL_I2C_Master_Receive(i2c_handle,receive_address, rx_data, MICS_RX_BUFFER_SIZE, I2C_TIMEOUT) == HAL_OK)
		{
			return true;
		}
    }
	return false;
};

/* --------------------------------------------------------------------------------------------------------------------
 * @param
 * 		command   - command to send to chip
 * 		interface - interface from hardware_defs.h
 * @ return
 * 		true  - success
 * 		false - fail
 * @ info Gets the MICS values based on command sent.
 --------------------------------------------------------------------------------------------------------------------*/
static uint8_t getSensorReading(mics_commands_t command, HW_I2C_t interface)
{

	uint8_t result =  MICS_COMMAND_FAIL;
	uint8_t data[7];

	result = mics_I2C_receive(MICS_PRIAMRY_I2C_ADDRESS, command, data, i2c_interfaces[interface]);

	if (result == MICS_COMMAND_SUCESS)
	{
		local_sensor_data.voc_value       = data[0];
		local_sensor_data.co2_level_value = data[1];
		local_sensor_data.raw_val_msb     = data[2];
		local_sensor_data.raw_value       = data[3];
		local_sensor_data.raw_val_lsb     = data[4];
		local_sensor_data.error_value     = data[5];
		local_sensor_data.crc_val         = data[6];
	};

	return result;
}

// --------------------------------------------------------------------------------------------------------------------
// This function triggers a new value read and get the CO2 value
// --------------------------------------------------------------------------------------------------------------------
static int getSensorReading_co2level (int32_t * resultPtr, int slot,sensorParamsPtr params)
{
	uint8_t result = MICS_COMMAND_FAIL;
	result = getSensorReading(MICS_VZ_89TE_CMD_GET_SENSOR_VALS, HW_I2C2);
	float co2_ppm = (((float)local_sensor_data.co2_level_value - 13.0) * ((float)1600/(float)229)) + (float)(400);
	*resultPtr = (int32_t) (co2_ppm * 100.00);

	return (result == MICS_COMMAND_SUCESS) ? SENSOR_SUCCESS : SENSOR_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// This function triggers a new value read and get the VOC value
// --------------------------------------------------------------------------------------------------------------------
static int getSensorReading_voclevel (int32_t * resultPtr, int slot,sensorParamsPtr params)
{
	uint8_t result = MICS_COMMAND_FAIL;
	float voc_ppm = ((float)local_sensor_data.voc_value - 13.0) * ((float)1000/(float)229);
	*resultPtr = (int32_t) (voc_ppm * 100.0);

	//return result;
	return SENSOR_SUCCESS;
}


