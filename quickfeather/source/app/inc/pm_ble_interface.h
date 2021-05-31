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

#define PM_BLE_PROPERTY_READ          0x02u
#define PM_BLE_PROPERTY_WRITE_NO_RESP 0x04u
#define PM_BLE_PROPERTY_WRITE         0x08u
#define PM_BLE_PROPERTY_NOTIFY        0x10u
#define PM_BLE_PROPERTY_INDICATE      0x20u

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------


typedef uint32_t pmBleDataType_t;
typedef uint16_t pmBleHandle_t;

typedef struct pmBleCharacteristic_s
{
	const char * uuid;   //!< String UUID (either 2-byte or 16-byte UUID).
	const uint8_t properties;  //!< 8-bit property field (0x02 = Read, 0x04 = Write Without Response, 0x08 = Write, 0x10 = Notify, 0x20 = Indicate).

	pmBleHandle_t handle; //!< RAM so that a handle can be allocated for the characteristic.
} pmBleCharacteristic_t;

typedef int (*pmBleRegisterServiceFcnPtr) (const char * uuid, pmBleCharacteristic_t * characteristics, uint16_t listLength);

typedef int (*pmBleWriteCharFcnPtr) (pmBleHandle_t characteristic, pmBleDataType_t data);

typedef int (*pmBleReadCharFcnPtr) (pmBleHandle_t characteristic, pmBleDataType_t * data);

typedef struct pmBleDriver_s
{
	pmBleRegisterServiceFcnPtr registerService;

    pmBleWriteCharFcnPtr writeChar;
    pmBleReadCharFcnPtr readChar;
} pmBleDriver_t;

#endif // PM_BLE_INTERFACE_H
