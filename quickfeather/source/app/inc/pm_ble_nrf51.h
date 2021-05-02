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

/**
* @brief Switch from command mode to UART mode or vice-versa. UART mode allows all UART characters to be transmitted
* when using the pmBleUartService_nRF51 driver, rather than a subset.
*
* @param uartMode Set to true to change to UART mode, false to change to command mode.
* @return bool True on success, false otherwise.
*/
bool pmBleSetMode_nRF51(bool uartMode);

// --------------------------------------------------------------------------------------------------------------------
// EXTERNS
// --------------------------------------------------------------------------------------------------------------------

//! Bluetooth driver functions for the nRF51 board.
extern const pmBleDriver_t pmBleDriver_nRF51;

//! UART driver which can be used to talk over the UART BLE Service on the nRF51.
extern const pmCoreUartDriver_t pmBleUartService_nRF51;


#endif // PM_BLE_NRF51_H
