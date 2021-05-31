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
int8_t bme680_initialize(bme_params * params);

// --------------------------------------------------------------------------------------------------------------------
// EXTERNS
// --------------------------------------------------------------------------------------------------------------------

//! Driver to get a raw gas reading from the sensor. No parameters required.
extern const sensorDriver_t sensorDriverRaw_bme680;

//! Driver to get a humidity reading in % relative humidity x1000. No parameters required.
extern const sensorDriver_t sensorDriverHumidity_bme680;

//! Driver to get a temperature reading in degree Celsius x100. No parameters required.
extern const sensorDriver_t sensorDriverTemperature_bme680;

//! Driver to get a pressure reading in Pascal. No parameters required.
extern const sensorDriver_t sensorDriverPressure_bme680;

//! Driver to get a CO2 reading in PPM x100. No parameters required.
extern const sensorDriver_t sensorDriverCO2_bme680;

//! Driver to get a Breath VOC reading in PPM x100. No parameters required.
extern const sensorDriver_t sensorDriverVOC_bme680;

//! Choose to init one of these init arrays.
extern bme_params bme_init_array[Number_of_interface];


#endif /* BME680_BME680_HELPER_H_ */
