/*
 * bme680_helper.h
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#ifndef BME680_BME680_HELPER_H_
#define BME680_BME680_HELPER_H_
#include "bme680.h"
#include "bme680_defs.h"
#include  "../sensor_interface.h"

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
void user_delay_ms(uint32_t period);
unsigned int getSensorReading_pressure (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params);
unsigned int getSensorReading_temperature (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params);
unsigned int getSensorReading_humidity (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params);
unsigned int getSensorReading_gas_resistance (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params);

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef enum
{
	BME_I2C1,

	Number_of_interface,
}interface_to_use;


typedef struct
{
	struct bme680_dev dev;
	uint8_t settings;
} bme_params;

#endif /* BME680_BME680_HELPER_H_ */
