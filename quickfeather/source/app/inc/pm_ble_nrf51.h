#ifndef PM_BLE_NRF51_H
#define PM_BLE_NRF51_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the bluetooth driver for an Adafruit nRF51.
//
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdint.h>

#include "pm_core_defs.h"

#include "pm_ble_interface.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------
/**
* @brief Initialize the nRF51. This function must be called in order to initialize this module.
* 
* @param driver The definition of the UART driver this module should use to talk to the physical device.
* @return int #PM_BLE_SUCCESS on success, #PM_BLE_FAILURE otherwise.
*/
int pmBleInit_nRF51(const pmCoreUartDriver_t * driver);

// --------------------------------------------------------------------------------------------------------------------
// EXTERNS
// --------------------------------------------------------------------------------------------------------------------
extern const pmCoreUartDriver_t pmBleUartService_nRF51;


#endif // PM_BLE_NRF51_H