/*
 * sensor_config.h
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

#ifndef SENSOR_CONFIG_H_
#define SENSOR_CONFIG_H_

#include "sensor_interface.h"

typedef enum sensorId_e
{
  HUMIDITY_SENSOR,
  PRESSURE_SENSOR,
  TEMP_SENSOR,
  CO2_SENSOR,
  TVOC_SENSOR,

  NUM_SENSOR_IDS
} sensorId_t;


extern const sensorModule_t sensorTable[NUM_SENSOR_IDS]; // Global list of supported sensors.


#endif /* SENSOR_CONFIG_H_ */
