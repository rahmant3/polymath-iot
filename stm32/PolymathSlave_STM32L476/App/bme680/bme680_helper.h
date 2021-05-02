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
#include  "sensor_interface.h"

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


// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
void user_delay_ms(uint32_t period);
int getSensorReading_pressure (int32_t * resultPtr, int slot,sensorParamsPtr params);
int getSensorReading_temperature (int32_t * resultPtr, int slot,sensorParamsPtr params);
int getSensorReading_humidity (int32_t * resultPtr, int slot,sensorParamsPtr params);
int getSensorReading_gas_resistance (int32_t * resultPtr, int slot,sensorParamsPtr params);

int8_t bme680_initialize(bme_params * params);

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------

extern bme_params bme_init_array[Number_of_interface];


#endif /* BME680_BME680_HELPER_H_ */