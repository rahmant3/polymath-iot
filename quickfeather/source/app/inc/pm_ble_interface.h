#ifndef PM_BLE_INTERFACE_H
#define PM_BLE_INTERFACE_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the interface a bluetooth driver must provide in order to be used within the polymath 
//    application.
//
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdint.h>

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define PM_BLE_SUCCESS 0x00 //!< Function return code on success.
#define PM_BLE_FAILURE 0x01 //!< Function return code on failure.

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef struct pmBleCharDefinition_s
{
  uint8_t properties;

  uint8_t minLen;
  uint8_t maxLen;
} pmBleCharDefinition_t;

typedef uint16_t pmBleHandle_t;

typedef pmBleHandle_t (*pmBleAddServiceFcnPtr) (uint16_t uuid);

typedef pmBleHandle_t (*pmBleAddServiceFcnPtr_128) (uint8_t uuid[16]);

typedef pmBleHandle_t (*pmBleAddCharFcnPtr) (uint16_t uuid, pmBleHandle_t service, pmBleCharDefinition_t * definition);

typedef int (*pmBleWriteCharFcnPtr) (uint16_t uuid, pmBleHandle_t service, void * data, uint16_t numBytes);

typedef int (*pmBleReadCharFcnPtr) (uint16_t uuid, pmBleHandle_t service, void * data, uint16_t numBytes);

typedef struct pmBleDriver_s
{
    pmBleAddServiceFcnPtr addService;
    pmBleAddServiceFcnPtr_128 addServic_128;

    pmBleAddCharFcnPtr addChar;
    pmBleWriteCharFcnPtr writeChar;
    pmBleReadCharFcnPtr readChar;
} pmBleDriver_t;

#endif // PM_BLE_INTERFACE_H