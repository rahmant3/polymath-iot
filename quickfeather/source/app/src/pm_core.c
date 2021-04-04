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

#include "pm_protocol.h"

#include "Fw_global_config.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define PM_CORE_RTOS_TASK_PERIOD_ms (5u)

#define PM_LED_BLINK_PERIOD_ms      pdMS_TO_TICKS(1000u)

#define ENABLE_SLAVE_LOOPBACK_TEST

#define BLUE_LED_GPIO  4
#define GREEN_LED_GPIO 5
#define RED_LED_GPIO   6

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION
// --------------------------------------------------------------------------------------------------------------------
static int pmMasterUartTx(uint8_t * data, uint8_t numBytes);
static int pmMasterUartRx(uint8_t * data, uint8_t numBytes);

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
    static int pmSlaveUartTx(uint8_t * data, uint8_t numBytes);
    static int pmSlaveUartRx(uint8_t * data, uint8_t numBytes);
#endif

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static bool g_initError = false;

/*static*/ pmProtocolContext_t g_pmUartMasterContext;
static pmProtocolDriver_t g_pmUartMasterDriver =
{
    .tx = pmMasterUartTx,
    .rx = pmMasterUartRx
};

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
    /*static*/ pmProtocolContext_t g_pmUartSlaveContext;
    static pmProtocolDriver_t g_pmUartSlaveDriver =
    {
        .tx = pmSlaveUartTx,
        .rx = pmSlaveUartRx
    };
#endif

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
static int pmMasterUartTx(uint8_t * data, uint8_t numBytes)
{
#if 0
	for (int ix = 0; ix < numBytes; ix++)
	{
	    uart_tx_raw_buf(PM_COPRO_UART, &data[ix], 1);
	    vTaskDelay(1); // Delay 1 ms.
	}
#endif

	uart_tx_raw_buf(PM_COPRO_UART, data, numBytes);

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
static int pmSlaveUartTx(uint8_t * data, uint8_t numBytes)
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

static volatile int testSend = 0;

static void pmCoreRtosTask(void * params)
{
    TickType_t lastWakeupTicks = xTaskGetTickCount();
    
    TickType_t lastLedBlinkTicks = lastWakeupTicks;
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

    while(1)
    {
        if (!g_initError)
        {
            TickType_t nowTicks = xTaskGetTickCount();
            uint32_t nowTicks_ms  = nowTicks * 1000 / configTICK_RATE_HZ;

            if ((nowTicks - lastLedBlinkTicks) > PM_LED_BLINK_PERIOD_ms)
            {
                ledOn = (ledOn == 0) ? 1 : 0;
                HAL_GPIO_Write(GREEN_LED_GPIO, ledOn);

                lastLedBlinkTicks = nowTicks;
            }

            pmProtocolPeriodic( nowTicks_ms, &g_pmUartMasterContext);
            
            #ifdef ENABLE_SLAVE_LOOPBACK_TEST
                pmProtocolPeriodic( nowTicks_ms, &g_pmUartSlaveContext);
            #endif
        }


        uint8_t usrBtn;
		HAL_GPIO_Read(0, &usrBtn);
		if (!usrBtn)
		{
			if (!testSend)
			{
				pm_test_send("test");
				testSend = 1;
			}
        }

        vTaskDelayUntil(&lastWakeupTicks, pdMS_TO_TICKS(PM_CORE_RTOS_TASK_PERIOD_ms));
    }
}

// --------------------------------------------------------------------------------------------------------------------
void pm_main()
{
    if (PM_PROTOCOL_SUCCESS != pmProtocolInit(&g_pmUartMasterDriver, &g_pmUartMasterContext))
    {
        g_initError = true;
    }
    #ifdef ENABLE_SLAVE_LOOPBACK_TEST
        if (PM_PROTOCOL_SUCCESS != pmProtocolInit(&g_pmUartSlaveDriver, &g_pmUartSlaveContext))
        {
            g_initError = true;
        }
    #endif

    if (pdPASS != xTaskCreate(pmCoreRtosTask, "Polymath Task", 1024, NULL, (configMAX_PRIORITIES / 2), NULL))
    {
        g_initError = true;
    }

    if (g_initError)
    {
        // Turn on the red LED.
        HAL_GPIO_Write(RED_LED_GPIO, 1);
    }
    else
    {
        // Turn on the green LED.
        HAL_GPIO_Write(GREEN_LED_GPIO, 1);
    }

}

