
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

#include <Fw_global_config.h>

#include <eoss3_hal_gpio.h>
#include <eoss3_hal_uart.h>

#if 0
BGLIB_DEFINE();

static uint8_t rxBuffer[BGLIB_MSG_MAXLEN];

static void pm_ble_tx(uint8 len1, uint8 * data1, uint16 len2, uint8 * data2)
{
	#if 1
		for (int ix = 0; ix < len1; ix++)
		{
			uart_tx_raw_buf(PM_BLE_UART, &data1[ix], 1);
			vTaskDelay(1); // Delay 1 ms.
		}
		for (int ix = 0; ix < len2; ix++)
		{
			uart_tx_raw_buf(PM_BLE_UART, &data2[ix], 1);
			vTaskDelay(1); // Delay 1 ms.
		}
	#else
		uart_tx_raw_buf(PM_BLE_UART, data1, len1);
		uart_tx_raw_buf(PM_BLE_UART, data2, len2);
	#endif
	}

static void pm_ble_rx()
{
	while (BGLIB_MSG_HEADER_LEN > uart_rx_available(PM_BLE_UART)) { }

	uart_rx_raw_buf(PM_BLE_UART, rxBuffer, BGLIB_MSG_HEADER_LEN);

	uint8_t msgLen = BGLIB_MSG_LEN(rxBuffer);

	while (msgLen > uart_rx_available(PM_BLE_UART)) { }

	uart_rx_raw_buf(PM_BLE_UART, &rxBuffer[msgLen], msgLen);

	uint8_t msgId = BGLIB_MSG_ID(rxBuffer);
}

void pm_ble_test()
{
	 BGLIB_INITIALIZE(pm_ble_tx);
	 dumo_cmd_system_hello();

	 pm_ble_rx();

}

#endif

#include "pm_ble_nrf51.h"

static int pmBleUartTx(uint8_t * data, uint8_t numBytes);
static int pmBleUartRx(uint8_t * data, uint8_t numBytes);

static int pmBleUartTx(uint8_t * data, uint8_t numBytes)
{
#if 1
	for (int ix = 0; ix < numBytes; ix++)
	{
	    uart_tx_raw_buf(PM_BLE_UART, &data[ix], 1);
	    vTaskDelay(1); // Delay 1 ms.
	}
#else
	uart_tx_raw_buf(PM_BLE_UART, data, numBytes);
#endif

    return numBytes;
}

// --------------------------------------------------------------------------------------------------------------------
static int pmBleUartRx(uint8_t * data, uint8_t numBytes)
{
    int result = 0;
    if (numBytes <= uart_rx_available(PM_BLE_UART))
    {
        result = uart_rx_raw_buf(PM_BLE_UART, data, numBytes);
    }
    return result;
}

static pmCoreUartDriver_t g_pmUartBleDriver =
{
    .tx = pmBleUartTx,
    .rx = pmBleUartRx
};


#define BLUE_LED_GPIO  4
#define GREEN_LED_GPIO 5
#define RED_LED_GPIO   6

void pm_ble_test()
{
	if (PM_BLE_SUCCESS == pmBleInit_nRF51(&g_pmUartBleDriver))
	{
		HAL_GPIO_Write(BLUE_LED_GPIO, 1);
	}
}
