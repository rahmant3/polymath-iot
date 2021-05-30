/*
 * sensor_interface.h
 *
 *  Created on: Apr. 6, 2021
 *      Author: hgopalsing
 */

#ifndef SENSOR_INTERFACE_H_
#define SENSOR_INTERFACE_H_

// --------------------------------------------------------------------------------------------------------------------
// This file holds type definitions for a generic sensor.
// --------------------------------------------------------------------------------------------------------------------
#define SENSOR_SUCCESS (0u)
#define SENSOR_FAILURE (1u)

typedef void * sensorParamsPtr; // Custom parameters that need to be passed to a driver function.

// --------------------------------------------------------------------------------------------------------------------
//  Function definition used for obtaining a sensor reading.
//
//  Returns zero on success, non-zero on failure.
// --------------------------------------------------------------------------------------------------------------------
typedef int (*getReadingFncPtr)(
  int32_t * resultPtr,   // The buffer to load the result to.
  int slot,              // The slot this sensor exists on (e.g. SPI SLOT 1, SPI SLOT 2, I2C SLOT 1, etc).
  sensorParamsPtr params // The parameters for this sensor.
);

// Structure containing driver functions for a sensor.
typedef struct sensorDriver_s
{
  getReadingFncPtr getReading; // Function used to obtain a sensor reading.
} sensorDriver_t;

// Structure containing the interface needed to control a sensor (i.e. drivers, parameters).
typedef struct sensorModule_s
{
  const sensorDriver_t * drivers;
  sensorParamsPtr params;
} sensorModule_t;


#endif /* SENSOR_INTERFACE_H_ */
