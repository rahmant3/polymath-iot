/*
 * bm680_helper.c
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <string.h> // For memcpy

#include <FreeRTOS.h>
#include <task.h>

#include "bme680_helper.h"

#if !(__has_include("bsec_integration.h"))
	#error "The BSEC library for BME680 must be downloaded in order to build this file (tested with V1.4.8.0): https://www.bosch-sensortec.com/software-tools/software/bsec/ \
	Add the following files to the bme680 folder:\
	- bsec_datatypes.h\
	- bsec_interface.h\
	- bsec_integration.h\
	- bsec_integration.c\
	- libalgobsec.a (from normal_version\bin\gcc\Cortex_M4)"

#else
	#include "bsec_integration.h"
#endif

#include "sensor_interface.h"
#include "main.h"

// --------------------------------------------------------------------------------------------------------------------
// PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------
static int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
static int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);

static void user_delay_ms(uint32_t period);

static int getSensorReading_pressure (int32_t * resultPtr, int slot,sensorParamsPtr params);
static int getSensorReading_temperature (int32_t * resultPtr, int slot,sensorParamsPtr params);
static int getSensorReading_humidity (int32_t * resultPtr, int slot,sensorParamsPtr params);
static int getSensorReading_gas_resistance (int32_t * resultPtr, int slot,sensorParamsPtr params);

static int getSensorReading_co2 (int32_t * resultPtr, int slot,sensorParamsPtr params);
static int getSensorReading_voc (int32_t * resultPtr, int slot,sensorParamsPtr params);
// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// CONSTANTS
// --------------------------------------------------------------------------------------------------------------------
const sensorDriver_t sensorDriverRaw_bme680 =
{
	.getReading = getSensorReading_gas_resistance
};
const sensorDriver_t sensorDriverHumidity_bme680 =
{
	.getReading = getSensorReading_humidity
};
const sensorDriver_t sensorDriverTemperature_bme680 =
{
	.getReading = getSensorReading_temperature
};
const sensorDriver_t sensorDriverPressure_bme680 =
{
	.getReading = getSensorReading_pressure
};
const sensorDriver_t sensorDriverCO2_bme680 =
{
	.getReading = getSensorReading_co2
};
const sensorDriver_t sensorDriverVOC_bme680 =
{
	.getReading = getSensorReading_voc
};

// --------------------------------------------------------------------------------------------------------------------
// VARIABLE
// --------------------------------------------------------------------------------------------------------------------
static struct bme680_field_data data;

// --------------------------------------------------------------------------------------------------------------------
// To be used with the bme_init_array when initializing the bme680 set sensor settings
// e.g -->  bme680_set_sensor_settings(bme_init_array[I2C].settings,bme_init_array[I2C].dev);
// --------------------------------------------------------------------------------------------------------------------
bme_params bme_init_array[Number_of_interface] =
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
static void user_delay_ms(uint32_t period)
{
	HAL_Delay(period);
}

static void user_delay_init_ms(uint32_t period)
{
	HAL_Delay(period);
}

static void user_delay_rtos_ms(uint32_t period)
{
#if 1
	if (period > 0)
	{
		vTaskDelay(period);
	}
#else
	HAL_Delay(period);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// I2C interface function -
// I2C1 -> read & write
// --------------------------------------------------------------------------------------------------------------------
static int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	uint8_t receive_device_address =  (dev_id << 1) | 1;

	uint8_t transmit_device_address =  (dev_id << 1);

	if ((HAL_I2C_Master_Transmit(&hi2c1, transmit_device_address, &reg_addr, 1, 100) == HAL_OK))
	{
		if (HAL_I2C_Master_Receive(&hi2c1, receive_device_address, reg_data, len, 100) == HAL_OK)
		{
			return 0;
		}
	}

	return 1;
}

uint8_t buffer[256];
static int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	uint8_t transmit_device_address =  (dev_id << 1);
	buffer[0] = reg_addr;
	(void)memcpy(&buffer[1], reg_data, len);
	if (HAL_I2C_Master_Transmit(&hi2c1, transmit_device_address, buffer, len + 1, 100) == HAL_OK)
	{
		return 0;
	}

	return 1;
};

/*!
 * @brief           Load previous library state from non-volatile memory
 *
 * @param[in,out]   state_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to state_buffer
 */
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // ...
    // Load a previous library state from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no state was available,
    // otherwise return length of loaded state string.
    // ...
    return 0;
}

/*!
 * @brief           Save library state to non-volatile memory
 *
 * @param[in]       state_buffer    buffer holding the state to be stored
 * @param[in]       length          length of the state string to be stored
 *
 * @return          none
 */
void state_save(const uint8_t *state_buffer, uint32_t length)
{
    // ...
    // Save the string some form of non-volatile memory, if possible.
    // ...
}

/*!
 * @brief           Load library config from non-volatile memory
 *
 * @param[in,out]   config_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to config_buffer
 */
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    // ...
    // Load a library config from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no config was available,
    // otherwise return length of loaded config string.
    // ...
    return 0;
}

/*!
 * @brief           Capture the system time in microseconds
 *
 * @return          system_current_time    current system timestamp in microseconds
 */
int64_t get_timestamp_us()
{
    return xTaskGetTickCount() * 1000;
}

/*!
 * @brief           Handling of the ready outputs
 *
 * @param[in]       timestamp       time in nanoseconds
 * @param[in]       iaq             IAQ signal
 * @param[in]       iaq_accuracy    accuracy of IAQ signal
 * @param[in]       temperature     temperature signal
 * @param[in]       humidity        humidity signal
 * @param[in]       pressure        pressure signal
 * @param[in]       raw_temperature raw temperature signal
 * @param[in]       raw_humidity    raw humidity signal
 * @param[in]       gas             raw gas sensor signal
 * @param[in]       bsec_status     value returned by the bsec_do_steps() call
 *
 * @return          none
 */

typedef struct bsecData_s
{
	int64_t timestamp;
	float iaq;
	uint8_t iaq_accuracy;
	float temperature;
	float humidity;
    float pressure;
    float raw_temperature;
    float raw_humidity;
    float gas;
    bsec_library_return_t bsec_status;
	float static_iaq;
	float co2_equivalent;
	float breath_voc_equivalent;
} bsecData_t;


static bsecData_t bmeData;

void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
     float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status,
     float static_iaq, float co2_equivalent, float breath_voc_equivalent)
{
	bmeData.timestamp = timestamp;
	bmeData.iaq = iaq;
	bmeData.iaq_accuracy = iaq_accuracy;
	bmeData.temperature = temperature;
	bmeData.humidity = humidity;
	bmeData.pressure = pressure;
	bmeData.raw_temperature = raw_temperature;
	bmeData.raw_humidity = raw_humidity;
	bmeData.gas = gas;
	bmeData.bsec_status = bsec_status;
	bmeData.static_iaq = static_iaq;
	bmeData.co2_equivalent = co2_equivalent;
	bmeData.breath_voc_equivalent = breath_voc_equivalent;
}


static void bsecTask(void * params)
{
	bsec_iot_loop(user_delay_rtos_ms, get_timestamp_us, output_ready, state_save, 10000);
}
// --------------------------------------------------------------------------------------------------------------------
// If result equals 0 then the initialization was successful
// Otherwise a negative integer is returned
// --------------------------------------------------------------------------------------------------------------------
int8_t bme680_initialize(bme_params * params)
{
	int8_t result = SENSOR_FAILURE;

    return_values_init ret;
	ret = bsec_iot_init(BSEC_SAMPLE_RATE_LP, 0.0f, user_i2c_write, user_i2c_read, user_delay_init_ms, state_load, config_load);
	if (ret.bme680_status)
	{
		/* Could not intialize BME680 */
		result = ret.bme680_status;
	}
	else if (ret.bsec_status)
	{
		/* Could not intialize BSEC library */
		result = ret.bsec_status;
	}
	else
	{
		if (pdPASS == xTaskCreate(bsecTask, "bsecTask", 1024, NULL, 4, NULL))
		{
			result = SENSOR_SUCCESS;
		}
	}
	return result;
}

// --------------------------------------------------------------------------------------------------------------------
//
// Gets new data for the sensor by updating the struct bme_data_field
// --------------------------------------------------------------------------------------------------------------------

static int UpdateSensorData(int slot, sensorParamsPtr params){

	 uint16_t meas_period;
	 int8_t result = BME680_E_COM_FAIL;

	 bme680_get_profile_dur(&meas_period, &(bme_init_array[slot].dev));
	 user_delay_ms(meas_period);
	 result = bme680_get_sensor_data(&data,&(bme_init_array[slot].dev));

	 if (result == BME680_OK)
	 {
		 result = bme680_set_sensor_mode(&(bme_init_array[slot].dev));
	 }

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the pressure - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 static int getSensorReading_pressure (int32_t * resultPtr, int slot,sensorParamsPtr params)
 {
	 #if 0
	 int8_t result = BME680_E_COM_FAIL;
	 result = UpdateSensorData(BME_I2C1,params);
	 *resultPtr = data.pressure;
	 #endif
	 int result = SENSOR_FAILURE;
	 if (bmeData.bsec_status == BSEC_OK)
	 {
		 *resultPtr = bmeData.pressure;
		 result = SENSOR_SUCCESS;
	 }

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the temperature - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 static int getSensorReading_temperature (int32_t * resultPtr, int slot,sensorParamsPtr params)
 {
#if 0
	 int8_t result = SENSOR_FAILURE;
	 if (BME680_OK == UpdateSensorData(BME_I2C1,params))
	 {
		 *resultPtr = data.temperature;
		 result = SENSOR_SUCCESS;
	 }
#endif
	 int result = SENSOR_FAILURE;
	 if (bmeData.bsec_status == BSEC_OK)
	 {
		 *resultPtr = bmeData.temperature * 1000;
		 result = SENSOR_SUCCESS;
	 }

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the gas-resistance - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
 static int getSensorReading_gas_resistance (int32_t * resultPtr, int slot,sensorParamsPtr params)
 {
#if 0
	 int8_t result = SENSOR_FAILURE;
	 if (BME680_OK == UpdateSensorData(BME_I2C1,params))
	 {
		 *resultPtr = data.gas_resistance;
	 	 result = SENSOR_SUCCESS;
	 }
#endif
	 int result = SENSOR_FAILURE;
	 if (bmeData.bsec_status == BSEC_OK)
	 {
		 *resultPtr = bmeData.gas * 1000;
		 result = SENSOR_SUCCESS;
	 }

	 return result;
 }

 // --------------------------------------------------------------------------------------------------------------------
 //
 // returns the humidity - to check the floating point enable
 // --------------------------------------------------------------------------------------------------------------------
static int getSensorReading_humidity(int32_t * resultPtr, int slot,sensorParamsPtr params)
{
#if 0
	 int result = SENSOR_FAILURE;
	 if (BME680_OK == UpdateSensorData(BME_I2C1,params))
	 {
		 *resultPtr = data.humidity;
	 	 result = SENSOR_SUCCESS;
	 }
#endif
	int result = SENSOR_FAILURE;
	if (bmeData.bsec_status == BSEC_OK)
	{
	 *resultPtr = bmeData.humidity * 1000;
	 result = SENSOR_SUCCESS;
	}
	return result;
}

static int getSensorReading_co2 (int32_t * resultPtr, int slot,sensorParamsPtr params)
{
 int result = SENSOR_FAILURE;
 if (bmeData.bsec_status == BSEC_OK)
 {
	 *resultPtr = bmeData.co2_equivalent * 100.0;
	 result = SENSOR_SUCCESS;
 }
 return result;
}

static int getSensorReading_voc (int32_t * resultPtr, int slot,sensorParamsPtr params)
{
 int result = SENSOR_FAILURE;
 if (bmeData.bsec_status == BSEC_OK)
 {
	 *resultPtr = bmeData.breath_voc_equivalent * 100.0;
	 result = SENSOR_SUCCESS;
 }
 return result;
}
