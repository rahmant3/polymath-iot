#include <stdio.h>
#include <stdlib.h>

#include "sensor_config.h"

#include "bme680_helper.h"
#include "micsvz89te.h"
#include "sensor_interface.h"


// Global list of sensors and sensor drivers.
const sensorModule_t sensorTable[NUM_SENSOR_IDS] =
{
  [HUMIDITY_SENSOR]   = { .drivers = &sensorDriverHumidity_bme680,    .params = NULL },
  [PRESSURE_SENSOR]   = { .drivers = &sensorDriverPressure_bme680,    .params = NULL },
  [TEMP_SENSOR]       = { .drivers = &sensorDriverTemperature_bme680, .params = NULL },
  [CO2_SENSOR]        = { .drivers = &sensorDriverCO2_micsvz89te,     .params = NULL },
  [TVOC_SENSOR]       = { .drivers = &sensorDriverVOC_micsvz89te,     .params = NULL },
  //[CO2_SENSOR]        = { .drivers = &sensorDriverCO2_bme680,     .params = NULL },
  //[TVOC_SENSOR]       = { .drivers = &sensorDriverVOC_bme680,     .params = NULL },

};

