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

#include <FreeRTOS.h>
#include <task.h>

#include "pm_protocol.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define START_BYTE   0x01

#define START_BYTE_OFFSET 0 // Byte offset into a packet containing the start byte.
#define LEN_BYTE_OFFSET   1 // Byte offset into a packet containing the length byte.

#define PM_PROTOCOL_ACK_BYTE 0x55

#define END_OF_PACKET_TIMEOUT_ms 5000

//#define DEBUG

#ifdef DEBUG
	#ifdef STM32L476xx
		extern void debug(const char * string);

		#define DEBUG_PRINTF(s) debug(s)
	#else
		#include <Fw_global_config.h>
		#include <dbg_uart.h>
		#define DEBUG_PRINTF(s) dbg_str(s)
	#endif
#else
	#define DEBUG_PRINTF(s)
#endif

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
static const uint8_t ackBytes[] = { PM_PROTOCOL_ACK_BYTE };

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
int pmProtocolInit(const pmCoreUartDriver_t * driver, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;
    if (   (NULL != context)
        && (NULL == context->driver) // If the driver is null, we can assume the module has not yet been initialized.
        && (NULL != driver)
        && (NULL != driver->rx)
        && (NULL != driver->tx))
    {
        context->driver = driver;

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

            context->txBuffer[START_BYTE_OFFSET] = START_BYTE;
            context->txBuffer[LEN_BYTE_OFFSET] = tx->numBytes + PM_NUM_OVERHEAD_BYTES; // Extra 3 bytes for start, length, checksum.

            (void)memcpy(&context->txBuffer[PM_NUM_OVERHEAD_BYTES - 1], tx->bytes, tx->numBytes);

            uint8_t checksum = 0x00;
            uint8_t len = context->txBuffer[LEN_BYTE_OFFSET];
            for (uint8_t index = 0; index < (len - 1); index++)
            {
                checksum += context->txBuffer[index];
            }
            context->txBuffer[len - 1] = ~checksum + 1;

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
        	context->rxState = WAITING_FOR_START;
            rc = PM_PROTOCOL_RX_TIMEOUT;
        }
        else if (rxState == CHECKSUM_ERROR)
        {
        	context->rxState = WAITING_FOR_START;
            rc = PM_PROTOCOL_CHECKSUM_ERROR;
        }
        else if (rxState == PAYLOAD_READY)
        {
            rx->numBytes = context->rxBuffer[LEN_BYTE_OFFSET] - PM_NUM_OVERHEAD_BYTES;
            (void)memcpy(rx->bytes, &context->rxBuffer[PM_NUM_OVERHEAD_BYTES - 1], rx->numBytes);

        	context->rxState = WAITING_FOR_START;
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
        	if (!context->txWaitingForAck)
        	{
        		// Transmit the first two bytes.
            	DEBUG_PRINTF("[pm_protocol_TX] Transmitted the first two bytes.\r\n");
				if (0 < context->driver->tx(context->txBuffer, LEN_BYTE_OFFSET + 1))
				{
					context->txWaitingForAck = true;
				}
        	}

        	if (context->txWaitingForAck)
        	{
        		uint8_t ack[sizeof(ackBytes)];
        		if ((sizeof(ackBytes) == context->driver->rx(ack, sizeof(ack))) && (0 == memcmp(ackBytes, ack, sizeof(ackBytes))))
        		{
        			// Ack received, send the rest of the bytes.
#if 0
        			// Delay a short period of time to give the receiver a chance to receive the bytes.
					vTaskDelay(pdMS_TO_TICKS(5));
#endif
                	DEBUG_PRINTF("[pm_protocol_TX] ACK received. Transmitted the rest of the bytes.\r\n");

        			context->txWaitingForAck = false;
        			if (0 < context->driver->tx(&context->txBuffer[LEN_BYTE_OFFSET + 1], context->txBuffer[LEN_BYTE_OFFSET] - 2))
        			{
        				context->txInProgress = false; // Transmit completed.
        			}
        		}
        	}
        }
        else
        {

			// Handle receives.
			switch(context->rxState)
			{
				case WAITING_FOR_START:
				{
					if (2 == context->driver->rx(context->rxBuffer, 2))
					{
						if (context->rxBuffer[START_BYTE_OFFSET] == START_BYTE)
						{
							DEBUG_PRINTF("[pm_protocol_RX] Received a start byte.\r\n");
							context->rxStartTicks = ticks_ms;
							context->rxBytes = 1;

							context->rxLen = context->rxBuffer[LEN_BYTE_OFFSET];
							if ((context->rxLen >= PM_NUM_OVERHEAD_BYTES) && (context->rxLen <= MAX_RX_BYTES))
							{

								DEBUG_PRINTF("[pm_protocol_RX] Received a length byte. Transmitting ACK.\r\n");
								context->rxBytes = 2;
								context->rxState = WAITING_FOR_DATA;


								if (sizeof(ackBytes) != context->driver->tx(ackBytes, sizeof(ackBytes)))
								{
									DEBUG_PRINTF("[pm_protocol_TX] Failed to transmit ACK.\r\n");
								}

								uint8_t rxBytes = context->driver->rx(&context->rxBuffer[context->rxBytes],
									context->rxLen - context->rxBytes);

								context->rxBytes += rxBytes;

							}
							else
							{
								DEBUG_PRINTF("[pm_protocol_RX] Received an invalid length byte.\r\n");
								context->rxState = CHECKSUM_ERROR;
							}
						}
					}
					break;
				}
				case WAITING_FOR_DATA:
				{
					if (context->rxBytes >= context->rxLen)
					{
						// We have the full payload ready. Verify the checksum.
						DEBUG_PRINTF("[pm_protocol_RX] Received a full payload. Processing.\r\n");

						uint8_t checksum = 0;
						for (uint8_t ix = 0; ix < context->rxLen; ix++)
						{
							checksum += context->rxBuffer[ix];
						}

						if (checksum == 0)
						{
							DEBUG_PRINTF("[pm_protocol_RX] Received a payload.\r\n");
							context->rxState = PAYLOAD_READY;
						}
						else
						{
							DEBUG_PRINTF("[pm_protocol_RX] Received a payload with checksum error.\r\n");
							context->rxState = CHECKSUM_ERROR;
						}
					}
					else
					{
						if ((ticks_ms - context->rxStartTicks) > END_OF_PACKET_TIMEOUT_ms)
						{
							// Timeout occurred.
							DEBUG_PRINTF("[pm_protocol_RX] Timeout occurred.\r\n");
							context->rxState = TIMEOUT_OCCURRED;
						}
						else
						{
							uint8_t rxBytes = context->driver->rx(&context->rxBuffer[context->rxBytes],
								context->rxLen - context->rxBytes);

							context->rxBytes += rxBytes;
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
}

// --------------------------------------------------------------------------------------------------------------------
int pmProtocolSend(pmCmdPayloadDefinition_t * tx, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;
    pmProtocolRawPacket_t rawTx;
    if (tx->commandCode < 0x7F) // This is a master transmitting.
    {
        switch (tx->commandCode)
        {
            case PM_CMD_PROTOCOL_ID: // Fall through
            case PM_CMD_CLUSTER_ID:  // Fall through
            case PM_CMD_GET_SENSORS: // Fall through
            {
                rawTx.bytes[0] = tx->commandCode;
                rawTx.numBytes = 1;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            case PM_CMD_WRITE_STATUS:
            {
                rawTx.bytes[0] = tx->commandCode;
                rawTx.bytes[1] = tx->writeStatus.status;

                rawTx.numBytes = 2;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            default:
                break;
        }
    }
    else // This is a slave transmitting.
    {
        switch (tx->commandCode & 0x7F)
        {
            case PM_CMD_PROTOCOL_ID:
            {
                rawTx.bytes[0] = tx->commandCode;
                rawTx.bytes[1] = tx->protocolInfo.protocolIdentifier & 0xFF;
                rawTx.bytes[2] = (tx->protocolInfo.protocolIdentifier >> 8)  & 0xFF;
                rawTx.bytes[3] = (tx->protocolInfo.protocolIdentifier >> 16) & 0xFF;
                rawTx.bytes[4] = (tx->protocolInfo.protocolIdentifier >> 24) & 0xFF;
                rawTx.numBytes = 5;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            case PM_CMD_CLUSTER_ID:
            {
                rawTx.bytes[0] = tx->commandCode;
                rawTx.bytes[1] = tx->clusterId.clusterIdentifier & 0xFF;
                rawTx.bytes[2] = (tx->clusterId.clusterIdentifier >> 8)  & 0xFF;
                rawTx.bytes[3] = (tx->clusterId.clusterIdentifier >> 16) & 0xFF;
                rawTx.bytes[4] = (tx->clusterId.clusterIdentifier >> 24) & 0xFF;
                rawTx.numBytes = 5;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            case PM_CMD_GET_SENSORS:
            {
                rawTx.bytes[0] = tx->commandCode;
                uint8_t ix = 1;
                for (uint8_t sensorIdx = 0; sensorIdx < tx->getSensors.numSensors; sensorIdx++)
                {
                    rawTx.bytes[ix++] = tx->getSensors.sensors[sensorIdx].sensorId & 0xFF;
                    rawTx.bytes[ix++] = (tx->getSensors.sensors[sensorIdx].sensorId >> 8) & 0xFF;

                    rawTx.bytes[ix++] = tx->getSensors.sensors[sensorIdx].data & 0xFF;
                    rawTx.bytes[ix++] = (tx->getSensors.sensors[sensorIdx].data >> 8) & 0xFF;
					rawTx.bytes[ix++] = (tx->getSensors.sensors[sensorIdx].data >> 16) & 0xFF;
					rawTx.bytes[ix++] = (tx->getSensors.sensors[sensorIdx].data >> 24) & 0xFF;
                }
                rawTx.numBytes = ix;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            case PM_CMD_WRITE_STATUS:
            {
                rawTx.bytes[0] = tx->commandCode;
                rawTx.bytes[1] = tx->writeStatus.status;

                rawTx.numBytes = 2;

                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            case PM_CMD_NEGATIVE_ACK:
            {
                rawTx.numBytes = 1;
                rc = PM_PROTOCOL_SUCCESS;
                break;
            }
            default:
                break;
        }
    }

    if (PM_PROTOCOL_SUCCESS == rc)
    {
        rc = pmProtocolSendPacket(&rawTx, context);
    }

    return rc;
}


// --------------------------------------------------------------------------------------------------------------------
int pmProtocolRead(pmCmdPayloadDefinition_t * rx, pmProtocolContext_t * context)
{
    int rc = PM_PROTOCOL_FAILURE;

    pmCmdPayloadDefinition_t localRx;
    pmProtocolRawPacket_t rawRx;

    rc = pmProtocolReadPacket(&rawRx, context);

    if (PM_PROTOCOL_SUCCESS == rc) // We received a packet.
    {
        rc = PM_PROTOCOL_RX_CMD_INVALID;

        localRx.commandCode = rawRx.bytes[0];
        if (localRx.commandCode < 0x7F) // This is a slave receiving a master packet.
        {
            switch (localRx.commandCode)
            {
                case PM_CMD_PROTOCOL_ID: // Fall through
                case PM_CMD_CLUSTER_ID:  // Fall through
                case PM_CMD_GET_SENSORS: // Fall through
                {
                    rc = PM_PROTOCOL_SUCCESS;
                    break;
                }
                case PM_CMD_WRITE_STATUS:
                {
                    localRx.writeStatus.status = rawRx.bytes[1];
                    rc = PM_PROTOCOL_SUCCESS;
                    break;
                }
                default:
                    break;
            }
        }
        else // This is a master receiving a slave packet.
        {
            switch (localRx.commandCode & 0x7F)
            {
                case PM_CMD_PROTOCOL_ID:
                {
                    if (rawRx.numBytes == 5)
                    {
                        localRx.protocolInfo.protocolIdentifier = rawRx.bytes[1]
                            + (rawRx.bytes[2] << 8)
                            + (rawRx.bytes[3] << 16)
                            + (rawRx.bytes[4] << 24);

                        rc = PM_PROTOCOL_SUCCESS;
                    }
                    break;
                }
                case PM_CMD_CLUSTER_ID:
                {
                    if (rawRx.numBytes == 5)
                    {
                        localRx.clusterId.clusterIdentifier = rawRx.bytes[1]
                            + (rawRx.bytes[2] << 8)
                            + (rawRx.bytes[3] << 16)
                            + (rawRx.bytes[4] << 24);

                        rc = PM_PROTOCOL_SUCCESS;
                    }
                    break;
                }
                case PM_CMD_GET_SENSORS:
                {
                    // Aside from the command byte, the rest of the bytes should be a multiple of PM_SIZE_OF_SENSOR_STRUCT.
                    if ((rawRx.numBytes > 0) && ((rawRx.numBytes - 1) % PM_SIZE_OF_SENSOR_STRUCT == 0))
                    {
                        localRx.getSensors.numSensors = (rawRx.numBytes - 1) / PM_SIZE_OF_SENSOR_STRUCT;

                        if (localRx.getSensors.numSensors < PM_MAX_SENSORS_PER_CLUSTER)
                        {
							for (uint8_t ix = 0; ix < localRx.getSensors.numSensors; ix++)
							{
								localRx.getSensors.sensors[ix].sensorId = rawRx.bytes[1 + (PM_SIZE_OF_SENSOR_STRUCT * ix)];
								localRx.getSensors.sensors[ix].sensorId += (rawRx.bytes[2 + (PM_SIZE_OF_SENSOR_STRUCT * ix)] << 8);

								localRx.getSensors.sensors[ix].data = rawRx.bytes[3 + (PM_SIZE_OF_SENSOR_STRUCT * ix)];
								localRx.getSensors.sensors[ix].data += (rawRx.bytes[4 + (PM_SIZE_OF_SENSOR_STRUCT * ix)] << 8);
								localRx.getSensors.sensors[ix].data += (rawRx.bytes[5 + (PM_SIZE_OF_SENSOR_STRUCT * ix)] << 16);
								localRx.getSensors.sensors[ix].data += (rawRx.bytes[6 + (PM_SIZE_OF_SENSOR_STRUCT * ix)] << 24);

								rc = PM_PROTOCOL_SUCCESS;
							}
                        }
                        else
                        {
                        	DEBUG_PRINTF("[pm_protocol_RX] Received an invalid sensor payload.\r\n");
                        }
                    }
                    break;
                }
                case PM_CMD_WRITE_STATUS:
                {
                    if (rawRx.numBytes == 2)
                    {
                        localRx.writeStatus.status = rawRx.bytes[1];
                        rc = PM_PROTOCOL_SUCCESS;
                    }
                    break;
                }
                case PM_CMD_NEGATIVE_ACK:
                {
                    rc = PM_PROTOCOL_SUCCESS;
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (PM_PROTOCOL_SUCCESS == rc)
    {
        (void)memcpy(rx, &localRx, sizeof(localRx));
    }

    return rc;
}


