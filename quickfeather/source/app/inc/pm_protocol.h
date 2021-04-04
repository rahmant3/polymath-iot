#ifndef PM_PROTOCOL_H
#define PM_PROTOCOL_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the core interface used to implement the polymath protocol. Example use:
//
/*
  // Define driver functions
  const pmProtocolDriver_t uart0Driver = 
  {
    .tx = function name,
    .rx = function name
  };
  // Declare RAM for the context.
  pmProtocolContext_t uart0Context;

  // Later...
  if (PM_PROTOCOL_SUCCESS == pmProtocolInit(&uart0Driver, &uart0Context))
  {
    ...
  }

  */
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdbool.h>

#include "pm_protocol_defs.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define PM_PROTOCOL_SUCCESS 0x00 //!< Function return code on success.
#define PM_PROTOCOL_FAILURE 0x01 //!< Function return code on failure.

#define PM_PROTOCOL_CHECKSUM_ERROR 0x02  //!< A packet read in progress failed due to a checksum error.
#define PM_PROTOCOL_RX_TIMEOUT     0x03  //!< A packet read in progress failed due to a timeout.
#define PM_PROTOCOL_TX_IN_PROGRESS 0x04  //!< A packet send failed due to another transmit in progress.
#define PM_PROTOCOL_RX_CMD_INVALID 0x05  //!< A packet read could not be decoded.

#define PM_MAX_PAYLOAD_BYTES 64

#define PM_NUM_OVERHEAD_BYTES 0x03
#define MAX_TX_BYTES         (PM_MAX_PAYLOAD_BYTES + PM_NUM_OVERHEAD_BYTES)
#define MAX_RX_BYTES         (PM_MAX_PAYLOAD_BYTES + PM_NUM_OVERHEAD_BYTES)

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
typedef int (*pmProtocolUartTxFcnPtr) (uint8_t * data, uint8_t numBytes);

/**
* @brief Driver function defined by the user, to receive UART bytes.
* 
* @param buffer The data buffer to load with the read bytes.
* @param numBytes Size of the data buffer.
* @return int The number of bytes loaded to the buffer.
*/
typedef int (*pmProtocolUartRxFcnPtr) (uint8_t * buffer, uint8_t numBytes);


struct pmProtocolDriver_s; // Forward declare
/**
* @brief Private context variables. The user must declare a structure in static RAM for this module to use. The module
*        takes care of initializing the RAM.
* 
*/
typedef struct pmProtocolContext_s
{
    const struct pmProtocolDriver_s * driver;

    bool txInProgress;
    uint8_t txBuffer[MAX_TX_BYTES];

    uint8_t rxState;
    uint8_t rxLen;
    uint8_t rxBytes;
    uint32_t rxStartTicks;
    uint8_t rxBuffer[MAX_RX_BYTES];
} pmProtocolContext_t;

/**
* @brief Parameters that allow this module to interact with the lower level hardware.
* 
*/
typedef struct pmProtocolDriver_s
{
  pmProtocolUartTxFcnPtr tx; //!< Function used for transmitting data over UART.
  pmProtocolUartRxFcnPtr rx; //!< Function used for receiving data over UART.
} pmProtocolDriver_t;


typedef struct pmProtocolRawPacket_s
{
  uint8_t bytes[PM_MAX_PAYLOAD_BYTES]; // The raw bytes to send.
  uint8_t numBytes;  // The number of bytes in this->bytes.
} pmProtocolRawPacket_t;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------

/**
* @brief Call in order to initialize this module.
* 
* @param driver Parameters that allow this module to interact with the hardware.
* @param context Context variables that can be used by this module.
* @return int #PM_PROTOCOL_SUCCESS on success, #PM_PROTOCOL_FAILURE otherwise.
*/
int pmProtocolInit(const pmProtocolDriver_t * driver, pmProtocolContext_t * context);

/**
* @brief This must be called periodically to allow this module to perform work.
* 
* @param ticks_ms Current ticks in ms to allow this module to perform timeout detection.
* @param context Context variables that can be used by this module.
*/
void pmProtocolPeriodic(uint32_t ticks_ms, pmProtocolContext_t * context);

/**
* @brief Call to queue up a payload for transmission.
* 
* @param tx The definition for the payload to send.
* @param context Context variables that can be used by this module.
* @return int #PM_PROTOCOL_SUCCESS on success. 
*             #PM_PROTOCOL_TX_IN_PROGRESS if a transmit is already in progress.
*             #PM_PROTOCOL_FAILURE otherwise.
*/
int pmProtocolSend(pmCmdPayloadDefinition_t * tx, pmProtocolContext_t * context);

/**
* @brief Call to read a payload from the receive queue.
* 
* @param rx The buffer to load the received payload into.
* @param context Context variables that can be used by this module.
* @return int #PM_PROTOCOL_SUCCESS if a message could be read.
*             #PM_PROTOCOL_FAILURE if a message could not be read.
*             #PM_PROTOCOL_CHECKSUM_ERROR if a message could be read but it had an invalid checksum.
*             #PM_PROTOCOL_RX_TIMEOUT if a message was in progress but failed due to a timeout.
*             #PM_PROTOCOL_RX_CMD_INVALID if a message was read but was formatted incorrectly.
*/
int pmProtocolRead(pmCmdPayloadDefinition_t * rx, pmProtocolContext_t * context);

/**
* @brief Call to queue up a raw packet for transmission.
* 
* @param tx The definition for the message to send.
* @param context Context variables that can be used by this module.
* @return int #PM_PROTOCOL_SUCCESS on success. 
*             #PM_PROTOCOL_TX_IN_PROGRESS if a transmit is already in progress.
*             #PM_PROTOCOL_FAILURE otherwise.
*/
int pmProtocolSendPacket(pmProtocolRawPacket_t * tx, pmProtocolContext_t * context);

/**
* @brief Call to read a raw packet from the receive queue.
* 
* @param rx The buffer to load the received message into.
* @param context Context variables that can be used by this module.
* @return int #PM_PROTOCOL_SUCCESS if a message could be read.
*             #PM_PROTOCOL_FAILURE if a message could not be read.
*             #PM_PROTOCOL_CHECKSUM_ERROR if a message could be read but it had an invalid checksum.
*             #PM_PROTOCOL_RX_TIMEOUT if a message was in progress but failed due to a timeout.
*/
int pmProtocolReadPacket(pmProtocolRawPacket_t * rx, pmProtocolContext_t * context);

#endif // PM_PROTOCOL_H
