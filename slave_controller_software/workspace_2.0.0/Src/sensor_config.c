#include <stdio.h>
#include <stdlib.h>
#include "Sensor_Interface.h"
#include "sensor_config.h"
#include "bme680/bme680_helper.h"

getReadingFncPtr getReading_CO2 = getSensorReading_gas_resistance;
getReadingFncPtr getReading_Humidity = getSensorReading_humidity;
getReadingFncPtr getReadingTemp = getSensorReading_temperature;
sensorDriver_t coSensor = {.getReading = getSensorReading_gas_resistance};
sensorDriver_t humidityDrivers = {.getReading = getSensorReading_humidity};
sensorDriver_t tempSensor = {.getReading =getSensorReading_temperature };

//sensorDriver_t getReading_H2O
// Declare parameters for all of the sensors in the sensor table.
//static const exampleH2O_t humidityParams = {0};
//static const exampleCO2_t co2SensorParams = {0};

// Global list of sensors and sensor drivers.
const sensorModule_t sensorTable[NUM_SENSOR_IDS] =
{
  [HUMIDITY_SENSOR]   = { .drivers = &humidityDrivers,  .params = NULL },
  [CO2_SENSOR]   = { .drivers = &coSensor, .params = NULL },
  [TEMP_SENSOR] = {.drivers = &tempSensor, .params = NULL },
};
