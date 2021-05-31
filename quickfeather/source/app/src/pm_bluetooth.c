
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

#include <Fw_global_config.h>

#include <eoss3_hal_gpio.h>
#include <eoss3_hal_uart.h>


#include "pm_ble_nrf51.h"

//! Service UUID used for the Polymath AIR cluster parameters. Note that the third and fourth bytes should not
//! conflict with the characteristic UUIDs used.
#define PM_AIR_CLUSTER_SERVICE_UUID "03-FA-FF-00-05-D5-4F-8D-B6-C7-17-9E-FC-E9-49-4D"

#define PM_AIR_CLUSTER_PRESSURE_UUID    "0x0001"
#define PM_AIR_CLUSTER_TEMPERATURE_UUID "0x0002"
#define PM_AIR_CLUSTER_HUMIDITY_UUID    "0x0003"
#define PM_AIR_CLUSTER_CO2_UUID         "0x0004" //!< CO2 concentration.
#define PM_AIR_CLUSTER_VOC_UUID         "0x0005" //!< VOC concentration.
#define PM_AIR_CLUSTER_ERO_UUID         "0x0006" //!< Estimated Room Occupancy.
#define PM_AIR_CLUSTER_EAQ_UUID         "0x0007" //!< Estimated Air Quality.

#define PM_AIR_CLUSTER_PRESSURE_IDX    0
#define PM_AIR_CLUSTER_TEMPERATURE_IDX 1
#define PM_AIR_CLUSTER_HUMIDITY_IDX    2
#define PM_AIR_CLUSTER_CO2_IDX         3
#define PM_AIR_CLUSTER_VOC_IDX         4
#define PM_AIR_CLUSTER_ERO_IDX         5
#define PM_AIR_CLUSTER_EAQ_IDX         6

pmBleCharacteristic_t pmAirClusterChars[] =
{
	[PM_AIR_CLUSTER_PRESSURE_IDX]    = {.uuid = PM_AIR_CLUSTER_PRESSURE_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_TEMPERATURE_IDX] = {.uuid = PM_AIR_CLUSTER_TEMPERATURE_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_HUMIDITY_IDX]    = {.uuid = PM_AIR_CLUSTER_HUMIDITY_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_CO2_IDX]         = {.uuid = PM_AIR_CLUSTER_CO2_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_VOC_IDX]         = {.uuid = PM_AIR_CLUSTER_VOC_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_ERO_IDX]         = {.uuid = PM_AIR_CLUSTER_ERO_UUID, .properties = PM_BLE_PROPERTY_READ },
	[PM_AIR_CLUSTER_EAQ_IDX]         = {.uuid = PM_AIR_CLUSTER_EAQ_UUID, .properties = PM_BLE_PROPERTY_READ },
};

const uint16_t numAirClusterEntries = sizeof(pmAirClusterChars) / sizeof(pmAirClusterChars[0]);

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


static int pmBleUartTx(const uint8_t * data, uint8_t numBytes);
static int pmBleUartRx(uint8_t * data, uint8_t numBytes);

static int pmBleUartTx(const uint8_t * data, uint8_t numBytes)
{
#if 1
	for (int ix = 0; ix < numBytes; ix++)
	{
	    uart_tx_raw_buf(PM_BLE_UART, &data[ix], 1);
	    vTaskDelay(10); // Delay 10 ms per byte.
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

void pm_ble_init()
{
	if (PM_BLE_SUCCESS == pmBleInit_nRF51(&g_pmUartBleDriver))
	{
		HAL_GPIO_Write(BLUE_LED_GPIO, 1);
	}
}

bool pm_ble_pair()
{
	bool result = false;
	if (PM_BLE_SUCCESS == pmBleDriver_nRF51.registerService(PM_AIR_CLUSTER_SERVICE_UUID, pmAirClusterChars, numAirClusterEntries))
	{
		result = true;
	}

	return result;
}

void pm_ble_test_update_char(uint16_t idx, pmBleDataType_t data)
{
	if (idx < numAirClusterEntries)
	{
		pmBleDriver_nRF51.writeChar(pmAirClusterChars[idx].handle, data);
	}
}
