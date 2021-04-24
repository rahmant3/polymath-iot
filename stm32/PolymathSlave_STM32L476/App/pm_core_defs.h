#ifndef PM_CORE_DEFS_H
#define PM_CORE_DEFS_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file has shared definitions for the polymath system.
//
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------

/**
* @brief Driver function defined by the user, to transmit UART bytes.
* 
* @param data The data bytes to send.
* @param numBytes The number of bytes to send.
* @return int The number of bytes sent.
*/
typedef int (*pmCoreUartTxFcnPtr) (const uint8_t * data, uint8_t numBytes);

/**
* @brief Driver function defined by the user, to receive UART bytes.
* 
* @param buffer The data buffer to load with the read bytes.
* @param numBytes Size of the data buffer.
* @return int The number of bytes loaded to the buffer.
*/
typedef int (*pmCoreUartRxFcnPtr) (uint8_t * buffer, uint8_t numBytes);


/**
* @brief Parameters that allow this module to interact with the lower level hardware.
* 
*/
typedef struct pmCoreUartDriver_s
{
  pmCoreUartTxFcnPtr tx; //!< Function used for transmitting data over UART.
  pmCoreUartRxFcnPtr rx; //!< Function used for receiving data over UART.
} pmCoreUartDriver_t;

#endif // PM_CORE_DEFS_H
