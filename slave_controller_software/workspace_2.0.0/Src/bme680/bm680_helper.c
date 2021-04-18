/*
 * bm680_helper.c
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include "bme680.h"
#include "bme680_defs.h"
#include "bme680_helper.h"
#include "stm32f4xx_hal.h"
#include "../sensor_interface.h"
#include "main.h"

// --------------------------------------------------------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// VARIABLE
// --------------------------------------------------------------------------------------------------------------------

static struct bme680_field_data data;

// --------------------------------------------------------------------------------------------------------------------
// To be used with the bme_init_array when initializing the bme680 set sensor settings
// e.g -->  bme680_set_sensor_settings(bme_init_array[I2C].settings,bme_init_array[I2C].dev);
// --------------------------------------------------------------------------------------------------------------------
static bme_params bme_init_array[Number_of_interface] =
{
		[BME_I2C1]=
		{
				.dev.dev_id = BME680_I2C_ADDR_PRIMARY,
				.dev.intf = BME680_I2C_INTF,
				.dev.read = user_i2c_read,
				.dev.delay_ms = user_delay_ms,
				.dev.write = user_i2c_write,
				.dev.amb_temp = 25,

				/* Set the temperature, pressure and humidity settings */
				.dev.tph_sett.os_hum = BME680_OS_2X,
				.dev.tph_sett.os_pres = BME680_OS_4X,
				.dev.tph_sett.os_temp = BME680_OS_8X,
				.dev.tph_sett.filter = BME680_FILTER_SIZE_3,

				/* Set the remaining gas sensor settings and link the heating profile */
				.dev.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS,

			    /* Create a ramp heat waveform in 3 steps */
				.dev.gas_sett.heatr_temp = 320, /* degree Celsius */
				.dev.gas_sett.heatr_dur = 150, /* milliseconds */
				.dev.power_mode = BME680_FORCED_MODE,

				.settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL
		        | BME680_GAS_SENSOR_SEL,
		},

};

// --------------------------------------------------------------------------------------------------------------------
// Callback for delay function for BME680 driver
// --------------------------------------------------------------------------------------------------------------------
void user_delay_ms(uint32_t period)
{
	HAL_Delay(period);
}

// --------------------------------------------------------------------------------------------------------------------
// I2C interface function -
// I2C1 -> read & write
// --------------------------------------------------------------------------------------------------------------------
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	uint8_t receive_device_address =  (dev_id << 1) | 1;

	uint8_t transmit_device_address =  (dev_id << 1);

	if ((HAL_I2C_Master_Transmit(&hi2c1, transmit_device_address, &reg_addr, 1, 100) == HAL_OK))
	{
		if (HAL_I2C_Master_Receive(&hi2c1, receive_device_address, reg_data, len, 100) == HAL_OK)
		{
			return 1;
		}
	}

	return 0;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	uint8_t transmit_device_address =  (dev_id << 1);

	if ((HAL_I2C_Master_Transmit(&hi2c1, transmit_device_address, &reg_addr, 1, 100) == HAL_OK))
	{
		if (HAL_I2C_Master_Transmit(&hi2c1, transmit_device_address, reg_data, len, 100) == HAL_OK)
		{
			return 1;
		}
	}

	return 0;
};


// --------------------------------------------------------------------------------------------------------------------
// If result equals 0 then the initialization was successful
// Otherwise a negative integer is returned
// --------------------------------------------------------------------------------------------------------------------

int8_t bme680_initialize(bme_params * params){

	int8_t result = BME680_E_COM_FAIL;
	result = bme680_init(&(params->dev));

	if (result == BME680_OK)
	{
		result = bme680_set_sensor_settings(params->settings,&(params->dev));
		if (result == BME680_OK)
		{
			result = bme680_set_sensor_mode(&(params->dev));
		}
	}
	return result;
}

// --------------------------------------------------------------------------------------------------------------------
//
// Gets new data for the sensor by updating the struct bme_data_field
// --------------------------------------------------------------------------------------------------------------------

 int UpdateSensorData(int slot, sensorParamsPtr params){

	 uint16_t meas_period;
	 int8_t result = BME680_E_COM_FAIL;

	 result = bme680_set_sensor_settings(bme_init_array[slot].settings,&(bme_init_array[slot].dev));
	 if (result == BME680_OK)
	 {
		 result = bme680_set_sensor_mode(&(bme_init_array[slot].dev));
		 if (result == BME680_OK)
		 {
			 bme680_get_profile_dur(&meas_period, &(bme_init_array[slot].dev));
			 user_delay_ms(meas_period);
			 result = bme680_get_sensor_data(&data,&(bme_init_array[slot].dev));
			 //result = bme680_set_sensor_mode(&(bme_init_array[slot].dev));
		 }
	 }
	 return result;
 }

 // is the size suppose to indicate what type of return we expect?

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the pressure - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 unsigned int getSensorReading_pressure (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params)
 {
	 int8_t result = BME680_E_COM_FAIL;
	 result = UpdateSensorData(slot,params);
	 *((float *)resultPtr) = (data.pressure)/100.0f;

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the temperature - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 unsigned int getSensorReading_temperature (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params)
 {
	 int8_t result = BME680_E_COM_FAIL;
	 result = UpdateSensorData(slot,params);
	 *((float *)resultPtr) = (data.temperature)/100.0f;

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the gas-resistance - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 unsigned int getSensorReading_gas_resistance (void (*resultPtr), int resultSize, int slot,sensorParamsPtr params)
 {
	 int8_t result = BME680_E_COM_FAIL;
	 result = UpdateSensorData(slot,params);
	 *((uint32_t *)resultPtr) = data.gas_resistance;

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the humidity - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 unsigned int getSensorReading_humidity(void (*resultPtr), int resultSize, int slot,sensorParamsPtr params)
 {
	 int8_t result = BME680_E_COM_FAIL;
	 UpdateSensorData(slot,params);
	 *((float *)resultPtr) = (data.humidity)/1000.0f;

	 return result;
 }
