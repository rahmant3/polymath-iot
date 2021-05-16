/*
 * sensor_config.h
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

#ifndef SENSOR_CONFIG_H_
#define SENSOR_CONFIG_H_

typedef enum sensorId_e
{
  HUMIDITY_SENSOR,
  CO2_SENSOR,
  TEMP_SENSOR,

  NUM_SENSOR_IDS
} sensorId_t;


extern const sensorModule_t sensorTable[NUM_SENSOR_IDS]; // Global list of supported sensors.


#endif /* SENSOR_CONFIG_H_ */