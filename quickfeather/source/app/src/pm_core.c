// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   Implements the logic for the polymath application.
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
// System includes
#include <stdbool.h>
#include <string.h>

// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// QORC includes
#include <dbg_uart.h>
#include <eoss3_hal_gpio.h>
#include <eoss3_hal_fpga_usbserial.h>
#include <eoss3_hal_uart.h>

#include "pm_core.h"

#include "Fw_global_config.h"
#include "pm_protocol.h"
#include "pm_ble_nrf51.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define PM_CORE_RTOS_TASK_PERIOD_ms (5u)

#define PM_LED_BLINK_PERIOD_ms      (1000u)
#define PM_TRAINING_PERIOD_ms       (1000u) //!< 1 Hz
//#define PM_TRAINING_PERIOD_ms       (100u) //!< 10 Hz

#define ENABLE_SLAVE_LOOPBACK_TEST
#define DEFAULT_MODE PM_MODE_TEST_SLAVE

#define BLUE_LED_GPIO  4
#define GREEN_LED_GPIO 5
#define RED_LED_GPIO   6

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION
// --------------------------------------------------------------------------------------------------------------------
static int pmMasterUartTx(const uint8_t * data, uint8_t numBytes);
static int pmMasterUartRx(uint8_t * data, uint8_t numBytes);

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
    static int pmSlaveUartTx(const uint8_t * data, uint8_t numBytes);
    static int pmSlaveUartRx(uint8_t * data, uint8_t numBytes);
#endif

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static pmCoreModes_t g_currentMode = PM_MODE_STARTUP;

/*static*/ pmProtocolContext_t g_pmUartMasterContext;
static pmCoreUartDriver_t g_pmUartMasterDriver =
{
    .tx = pmMasterUartTx,
    .rx = pmMasterUartRx
};

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
    /*static*/ pmProtocolContext_t g_pmUartSlaveContext;
    static pmCoreUartDriver_t g_pmUartSlaveDriver =
    {
        .tx = pmSlaveUartTx,
        .rx = pmSlaveUartRx
    };
#endif

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
static int pmMasterUartTx(const uint8_t * data, uint8_t numBytes)
{
#if 0
	for (int ix = 0; ix < numBytes; ix++)
	{
	    uart_tx_raw_buf(PM_COPRO_UART, &data[ix], 1);
	    vTaskDelay(1); // Delay 1 ms.
	}
#else
	uart_tx_raw_buf(PM_COPRO_UART, data, numBytes);
#endif

    return numBytes;
}

// --------------------------------------------------------------------------------------------------------------------
static int pmMasterUartRx(uint8_t * data, uint8_t numBytes)
{
    int result = 0;
    if (numBytes <= uart_rx_available(PM_COPRO_UART))
    {
        result = uart_rx_raw_buf(PM_COPRO_UART, data, numBytes);
    }
    return result;
}

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
// --------------------------------------------------------------------------------------------------------------------
static int pmSlaveUartTx(const uint8_t * data, uint8_t numBytes)
{
    uart_tx_raw_buf(PM_BLE_UART, data, numBytes);

    return numBytes;
}

// --------------------------------------------------------------------------------------------------------------------
static int pmSlaveUartRx(uint8_t * data, uint8_t numBytes)
{
    int result = 0;
    if (numBytes <= uart_rx_available(PM_BLE_UART))
    {
        result = uart_rx_raw_buf(PM_BLE_UART, data, numBytes);
    }
    return result;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
void pm_test_send(const char *s)
{
    pmProtocolRawPacket_t masterTx;
    masterTx.numBytes = strnlen(s, PM_MAX_PAYLOAD_BYTES);
    (void)memcpy(masterTx.bytes, s, masterTx.numBytes);

    if (PM_PROTOCOL_SUCCESS == pmProtocolSendPacket(&masterTx, &g_pmUartMasterContext))
    {
        dbg_str("Successfully sent the test string on the master node: ");
        dbg_str(s);
        dbg_str("\r\n");
    }
    else
    {
        dbg_str("Error: Failed to send the test string on the master node.\r\n");
    }
}

// --------------------------------------------------------------------------------------------------------------------
void pm_ble_init(void);
bool pm_ble_pair(void);

static volatile int testSend = 0;

static void pmCoreUartPassThrough(uint8_t consoleId, uint8_t otherId, uint8_t * escCount)
{
	uint8_t ch;

	// Pass through console characters to the BLE module.
	while (uart_rx_available(consoleId) > 0)
	{
		uart_rx_raw_buf(consoleId, &ch, 1);

		if (NULL != escCount)
		{
			if (ch == '\n')
			{
				*escCount = *escCount + 1;
			}
			else
			{
				*escCount = 0;
			}
		}

		uart_tx_raw_buf(otherId, &ch, 1);
	}

	// Pass through BLE characters to the console.
	while (uart_rx_available(otherId) > 0)
	{
		uart_rx_raw_buf(otherId, &ch, 1);
		uart_tx_raw_buf(consoleId, &ch, 1);
	}
}

static bool g_waitingForHandshake = true;
static void pmCoreNotifyStatus(uint8_t status)
{
	static bool pendingData = false;

	pmCmdPayloadDefinition_t masterTx;
	pmCmdPayloadDefinition_t masterRx;

	if (!pendingData)
	{
		// Issue a request for the data.
		masterTx.commandCode = PM_CMD_WRITE_STATUS;
		masterTx.writeStatus.status = status;
		if (PM_PROTOCOL_SUCCESS == pmProtocolSend(&masterTx, &g_pmUartMasterContext))
		{
			pendingData = true;
		}
	}
	else
	{
		int readResult = pmProtocolRead(&masterRx, &g_pmUartMasterContext);
		if (PM_PROTOCOL_SUCCESS == readResult)
		{
			pendingData = false;
			switch (masterRx.commandCode & 0x7F)
			{
				case PM_CMD_NEGATIVE_ACK:
					// Received an unexpected NACK. Retry on the next pass.
					break;
				case PM_CMD_WRITE_STATUS:
					// Handshake completed.
					if (masterRx.writeStatus.status == status)
					{
						dbg_str("[pm_core] Handshake completed.\r\n");
						g_waitingForHandshake = false;
					}
					break;
				default:
					// Was not expecting this code.
					break;
			}
		}
		else if (
				(PM_PROTOCOL_RX_CMD_INVALID == readResult)
				|| (PM_PROTOCOL_RX_TIMEOUT == readResult)
				|| (PM_PROTOCOL_RX_CMD_INVALID == readResult)
		)
		{
			// Read completed but there isn't a valid packet.
			pendingData = false;
		}
		// Else, still waiting to read a response to the read request.
	}
}


static pmCmdGetSensors_t sensorData;
static void pmCoreReadSensorData(pmCmdGetSensors_t * data)
{
	static bool pendingData = false;

	pmCmdPayloadDefinition_t masterTx;
	pmCmdPayloadDefinition_t masterRx;

	if (!pendingData)
	{
		// Issue a request for the data.
		masterTx.commandCode = PM_CMD_GET_SENSORS;
		if (PM_PROTOCOL_SUCCESS == pmProtocolSend(&masterTx, &g_pmUartMasterContext))
		{
			pendingData = true;
		}
	}
	else
	{
		int readResult = pmProtocolRead(&masterRx, &g_pmUartMasterContext);
		if (PM_PROTOCOL_SUCCESS == readResult)
		{
			pendingData = false;
			switch (masterRx.commandCode & 0x7F)
			{
				case PM_CMD_NEGATIVE_ACK:
					// Received an unexpected NACK. Retry on the next pass.
					break;
				case PM_CMD_GET_SENSORS:
					// Data received -- copy it to the result buffer.
					memcpy(data, &masterRx.getSensors, sizeof(*data));
					break;
				default:
					// Was not expecting this code.
					break;
			}
		}
		else if (
				(PM_PROTOCOL_RX_CMD_INVALID == readResult)
				|| (PM_PROTOCOL_RX_TIMEOUT == readResult)
				|| (PM_PROTOCOL_RX_CMD_INVALID == readResult)
		)
		{
			// Read completed but there isn't a valid packet.
			pendingData = false;
		}
		// Else, still waiting to read a response to the read request.
	}
}

static void pmCoreRtosTask(void * params)
{
    TickType_t lastWakeupTicks = xTaskGetTickCount();
    
    TickType_t lastLedBlinkTicks = lastWakeupTicks;
    TickType_t lastTrainingTicks = lastWakeupTicks;
    uint8_t ledOn = 1;


    // Flush any existing bytes.
	uint8_t dummy;

	while (uart_rx_available(UART_ID_HW) > 0)
	{
		uart_rx_raw_buf(UART_ID_HW, &dummy, 1);
	}
	while (uart_rx_available(UART_ID_FPGA) > 0)
	{
		uart_rx_raw_buf(UART_ID_FPGA, &dummy, 1);
	}
	while (uart_rx_available(UART_ID_FPGA_UART1) > 0)
	{
		uart_rx_raw_buf(UART_ID_FPGA_UART1, &dummy, 1);
	}

	// Used for training.
    const char * connectString = "connect";
    const int connectLen = sizeof(connectString) - 1;
    const char * jsonString =  "{\"sample_rate\":1,"
    		   	   	   	   	   "\"samples_per_packet\":3,"
							   "\"column_location\":{"
    						   "  \"Temperature\":0,"
    						   "  \"Humidity\":1,"
    						   "  \"Pressure\":2"
    		   	   	   	   	   "}"
    						   "}\r\n" ;

    int jsonLen = strlen(jsonString);

	pm_ble_init();

	uint8_t escCount = 0;
    while(1)
    {
        TickType_t nowTicks = xTaskGetTickCount();
        uint32_t nowTicks_ms  = nowTicks * 1000 / configTICK_RATE_HZ;

    	if (g_currentMode == PM_MODE_TEST_BLE)
    	{
    		// Blink the blue LED.
            if ((nowTicks - lastLedBlinkTicks) > pdMS_TO_TICKS(PM_LED_BLINK_PERIOD_ms))
            {
        		HAL_GPIO_Write(GREEN_LED_GPIO, 0);
        		HAL_GPIO_Write(RED_LED_GPIO, 0);

                ledOn = (ledOn == 0) ? 1 : 0;
                HAL_GPIO_Write(BLUE_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }

            pmCoreUartPassThrough(DEBUG_UART, PM_BLE_UART, &escCount);

            // If the user has entered 2 '\n' characters in a row, we'll exit this mode.
			if (escCount > 1)
			{
				g_currentMode = DEFAULT_MODE;
				escCount = 0;
			}
    	}
    	else if (g_currentMode == PM_MODE_TEST_SLAVE)
        {
    		// Blink the green LED.
            if ((nowTicks - lastLedBlinkTicks) > pdMS_TO_TICKS(PM_LED_BLINK_PERIOD_ms))
            {
        		HAL_GPIO_Write(BLUE_LED_GPIO, 0);
        		HAL_GPIO_Write(RED_LED_GPIO, 0);

                ledOn = (ledOn == 0) ? 1 : 0;
                HAL_GPIO_Write(GREEN_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }

            pmProtocolPeriodic( nowTicks_ms, &g_pmUartMasterContext);
            
            #ifdef ENABLE_SLAVE_LOOPBACK_TEST
                pmProtocolPeriodic( nowTicks_ms, &g_pmUartSlaveContext);
            #endif
        }
    	else if (g_currentMode == PM_MODE_TEST_TRAINING)
        {
    		// Blink the amber LED.
            if ((nowTicks - lastLedBlinkTicks) > pdMS_TO_TICKS(PM_LED_BLINK_PERIOD_ms))
            {
        		HAL_GPIO_Write(BLUE_LED_GPIO, 0);

                ledOn = (ledOn == 0) ? 1 : 0;
                HAL_GPIO_Write(GREEN_LED_GPIO, ledOn);
        		HAL_GPIO_Write(RED_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }

            pmProtocolPeriodic( nowTicks_ms, &g_pmUartMasterContext);
            pmCoreReadSensorData(&sensorData);

            char buffer[512];

            static bool ssi_connected = false;
            if ((nowTicks - lastTrainingTicks) > pdMS_TO_TICKS(PM_TRAINING_PERIOD_ms))
            {
            	if (!ssi_connected)
            	{
            		if (connectLen <= pmBleUartService_nRF51.rx(buffer, connectLen))
            		{
            			ssi_connected = true;
            		}
            		else
            		{
            			sprintf(buffer, "[pm_core] Sending json with len %d.\r\n", jsonLen);
            			dbg_str(buffer);
            			dbg_str(jsonString);
            			dbg_str("\n");
            			pmBleUartService_nRF51.tx(jsonString, jsonLen);
            		}
            	}
            	else
            	{
            		sprintf(buffer, "%d,%d,%d", sensorData.sensors[0].data,sensorData.sensors[1].data, sensorData.sensors[2].data);
            		pmBleUartService_nRF51.tx(buffer, strlen(buffer));
            	}
            }
        }
    	else if (g_currentMode == PM_MODE_PAIRING)
    	{
    		// Turn off the LED to indicate pairing.
    		HAL_GPIO_Write(GREEN_LED_GPIO, 0);
    		HAL_GPIO_Write(RED_LED_GPIO, 0);
            HAL_GPIO_Write(BLUE_LED_GPIO, 0);

    		if (pm_ble_pair())
    		{
    			g_currentMode = PM_MODE_NORMAL;
    		}
    		else
    		{
    			g_currentMode = PM_MODE_ERROR;
    		}
    	}
    	else if (g_currentMode == PM_MODE_NORMAL)
    	{
    		// Blink the white LED.
            if ((nowTicks - lastLedBlinkTicks) > pdMS_TO_TICKS(PM_LED_BLINK_PERIOD_ms))
            {
                ledOn = (ledOn == 0) ? 1 : 0;
                HAL_GPIO_Write(GREEN_LED_GPIO, ledOn);
        		HAL_GPIO_Write(BLUE_LED_GPIO, ledOn);
        		HAL_GPIO_Write(RED_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }

            pmProtocolPeriodic( nowTicks_ms, &g_pmUartMasterContext);
            if (g_waitingForHandshake)
            {
            	pmCoreNotifyStatus(0); // Indicate we are ready to go.
            }
            else
            {
            	pmCoreReadSensorData(&sensorData);
            	// TODO: Publish the data over BLE.

            }
    	}
    	else if (g_currentMode == PM_MODE_ERROR)
    	{
    		// Blink the red LED.
            if ((nowTicks - lastLedBlinkTicks) > pdMS_TO_TICKS(PM_LED_BLINK_PERIOD_ms))
            {
                HAL_GPIO_Write(GREEN_LED_GPIO, 0);
        		HAL_GPIO_Write(BLUE_LED_GPIO, 0);

                ledOn = (ledOn == 0) ? 1 : 0;
        		HAL_GPIO_Write(RED_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }


            pmProtocolPeriodic( nowTicks_ms, &g_pmUartMasterContext);
            pmCoreNotifyStatus(1); // Indicate we are in an error state.
    	}
        vTaskDelayUntil(&lastWakeupTicks, pdMS_TO_TICKS(PM_CORE_RTOS_TASK_PERIOD_ms));
    }
}

// --------------------------------------------------------------------------------------------------------------------
void pm_main()
{
    if (PM_PROTOCOL_SUCCESS != pmProtocolInit(&g_pmUartMasterDriver, &g_pmUartMasterContext))
    {
    	g_currentMode = PM_MODE_ERROR;
    }
    #ifdef ENABLE_SLAVE_LOOPBACK_TEST
        if (PM_PROTOCOL_SUCCESS != pmProtocolInit(&g_pmUartSlaveDriver, &g_pmUartSlaveContext))
        {
        	g_currentMode = PM_MODE_ERROR;
        }
    #endif

    if (pdPASS != xTaskCreate(pmCoreRtosTask, "Polymath Task", 2048, NULL, (configMAX_PRIORITIES / 2) + 1, NULL))
    {
    	g_currentMode = PM_MODE_ERROR;
    }

    if (g_currentMode == PM_MODE_ERROR)
    {
        // Turn on the red LED.
        HAL_GPIO_Write(RED_LED_GPIO, 1);
    }
    else
    {
    	g_currentMode = DEFAULT_MODE;
        // Turn on the green LED.
        HAL_GPIO_Write(GREEN_LED_GPIO, 1);
    }

}

void pmSetMode(pmCoreModes_t mode)
{
	g_currentMode = mode;
}
pmCoreModes_t pmGetMode()
{
	return g_currentMode;
}

