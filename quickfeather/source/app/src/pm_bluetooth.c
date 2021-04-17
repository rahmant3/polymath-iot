
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

#include <dumo_bglib.h>
#include <Fw_global_config.h>

#include <eoss3_hal_uart.h>

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
