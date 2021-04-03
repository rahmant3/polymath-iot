// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file implements the interface in pm_protocol.h.
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <string.h>

#include "pm_protocol.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define START_BYTE   0x01
#define END_OF_PACKET_TIMEOUT_ms 250

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef enum pmProtocolRxStates_e
{
    WAITING_FOR_START,
    WAITING_FOR_LEN,
    WAITING_FOR_DATA,

    PAYLOAD_READY,
    TIMEOUT_OCCURRED,
    CHECKSUM_ERROR
} pmProtocolRxStates_t;

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static pmProtocolContext_t g_pmProtocolContext;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
int pmProtocolInit(const pmProtocolDriver_t * driver, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;
    if (   (NULL != context) 
        && (NULL == context->driver) // If the driver is null, we can assume the module has not yet been initialized.
        && (NULL != driver) 
        && (NULL != driver->rx) 
        && (NULL != driver->tx))
    {
        context->driver = driver;

        (void)memset(&g_pmProtocolContext, 0, sizeof(g_pmProtocolContext));

        rc = PM_PROTOCOL_SUCCESS;
    }

    return rc;
}

// --------------------------------------------------------------------------------------------------------------------
int pmProtocolSendPacket(pmProtocolRawPacket_t * tx, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;
    if ((NULL != context->driver) && (NULL != tx))
    {
        if (!context->txInProgress)
        {
            context->txInProgress = true;
            
            uint8_t ix = 0;
            context->txBuffer[0] = START_BYTE;
            context->txBuffer[1] = tx->numBytes + PM_NUM_OVERHEAD_BYTES; // Extra 3 bytes for start, length, checksum.

            (void)memcpy(&context->txBuffer[PM_NUM_OVERHEAD_BYTES - 1], tx->bytes, tx->numBytes);
            
            uint8_t checksum = 0x00;
            uint8_t len = context->txBuffer[1];
            for (uint8_t index = 0; index < (len - 1); index++)
            {
                checksum += context->txBuffer[index];
            }
            context->txBuffer[ix++] = ~checksum + 1;

            rc = PM_PROTOCOL_SUCCESS;
        }
        else
        {
            rc = PM_PROTOCOL_TX_IN_PROGRESS;
        }
    }
    return rc;
}

// --------------------------------------------------------------------------------------------------------------------
int pmProtocolReadPacket(pmProtocolRawPacket_t * rx, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;
    
    if ((NULL != context->driver) && (NULL != rx))
    {
        pmProtocolRxStates_t rxState = context->rxState;
        if (rxState == TIMEOUT_OCCURRED)
        {
            rc = PM_PROTOCOL_RX_TIMEOUT;
        }
        else if (rxState == CHECKSUM_ERROR)
        {
            rc = PM_PROTOCOL_CHECKSUM_ERROR;
        }
        else if (rxState == PAYLOAD_READY)
        {
            rx->numBytes = context->rxBuffer[1] + PM_NUM_OVERHEAD_BYTES;
            (void)memcpy(rx->bytes, &context->rxBuffer[PM_NUM_OVERHEAD_BYTES - 1], rx->numBytes);

            rc = PM_PROTOCOL_SUCCESS;
        }
    }

    return rc;
}

// --------------------------------------------------------------------------------------------------------------------
void pmProtocolPeriodic(uint32_t ticks_ms, pmProtocolContext_t * context)
{
    if (NULL != context->driver)
    {
        // Handle transmits.
        if (context->txInProgress)
        {
            uint8_t len = context->txBuffer[1];
            
            if (0 < context->driver->tx(context->txBuffer, context->txBuffer[1]))
            {
                context->txInProgress = false; // Transmit completed.
            }
        }

        // Handle receives.
        switch(context->rxState)
        {
            case WAITING_FOR_START:
            {
                if (1 == context->driver->rx(context->rxBuffer, 1))
                {
                    if (context->rxBuffer[0] == START_BYTE)
                    {
                        context->rxStartTicks = ticks_ms;
                        context->rxBytes = 1;
                        context->rxState = WAITING_FOR_LEN;
                    }
                }
                break;
            }
            case WAITING_FOR_LEN:
            {
                if (1 == context->driver->rx(&context->rxBuffer[1], 1))
                {
                    context->rxLen = context->rxBuffer[1];
                    if (context->rxLen <= MAX_RX_BYTES)
                    {                        
                        context->rxBytes = 2;
                        context->rxState = WAITING_FOR_DATA;
                    }
                    else
                    {
                        context->rxState = CHECKSUM_ERROR;
                    }
                }
                else
                {
                    if ((context->rxStartTicks - ticks_ms) > END_OF_PACKET_TIMEOUT_ms)
                    {
                        // Timeout occurred.
                        context->rxState = TIMEOUT_OCCURRED;
                    }
                }
                break;
            }
            case WAITING_FOR_DATA:
            {
                uint8_t rxBytes = context->driver->rx(&context->rxBuffer[context->rxBytes], 
                    context->rxLen - context->rxBytes);

                context->rxBytes += rxBytes;
                if (context->rxBytes >= context->rxLen)
                {
                    // We have the full payload ready. Verify the checksum.
                    uint8_t checksum = 0;
                    for (uint8_t ix = 0; ix < context->rxLen; ix++)
                    {
                        checksum += context->rxBuffer[ix];
                    }

                    if (checksum == 0)
                    {
                        context->rxState = PAYLOAD_READY;
                    }
                    else
                    {
                        context->rxState = CHECKSUM_ERROR;
                    }
                }
                else
                {
                    if ((context->rxStartTicks - ticks_ms) > END_OF_PACKET_TIMEOUT_ms)
                    {
                        // Timeout occurred.
                        context->rxState = TIMEOUT_OCCURRED;
                    }
                }

                break;
            }
            case PAYLOAD_READY:    // Pass through
            case TIMEOUT_OCCURRED: // Pass through
            case CHECKSUM_ERROR:   // Pass through
            default:
                break;
        }
    }
}




