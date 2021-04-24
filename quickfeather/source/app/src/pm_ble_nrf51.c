// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file implements the interface in pm_ble_interface.h and pm_ble_nrf51.h.
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <dbg_uart.h>

#include "pm_ble_nrf51.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 128

#define MAX_AT_CMD_LEN TX_BUFFER_SIZE

#define STRLEN_UUID_16   (6) //!< A 16-bit UUID has 6 characters, e.g. 0x1234.
#define STRLEN_UUID_128  (16 * 3 - 1) //!< A 128-bit UUID has 3 characters per octet minus 1, e.g. 00-11-22-...-EE-FF.

#define DEFAULT_AT_TIMEOUT_ms 200

#define DBG_PRINT(s) dbg_str(s)
//#define DBG_PRINT(s)

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------

static int pmBleUartTx_nRF51(const uint8_t * bytes, uint8_t numBytes);
static int pmBleUartRx_nRF51(uint8_t * bytes, uint8_t numBytes);


static int pmBleRegisterService_nRF51(const char * uuid, pmBleCharacteristic_t * characteristics, uint16_t listLength);

static int pmBleWriteChar_nRF51(pmBleHandle_t characteristic, pmBleDataType_t data);
static int pmBleReadChar_nRF51(pmBleHandle_t characteristic, pmBleDataType_t * data);

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
const pmCoreUartDriver_t pmBleUartService_nRF51 =
{
    .tx = pmBleUartTx_nRF51,
    .rx = pmBleUartRx_nRF51
};

const pmBleDriver_t pmBleDriver_nRF51 =
{
	.registerService = pmBleRegisterService_nRF51,

	.writeChar = pmBleWriteChar_nRF51,
	.readChar = pmBleReadChar_nRF51
};

static const pmCoreUartDriver_t * g_uartDriver = NULL;


//! Buffer used by this module for handling receive bytes from the module.
static uint8_t g_rxBuffer[RX_BUFFER_SIZE];

//! Buffer used by this module for transmitting bytes to the module.
static uint8_t g_txBuffer[TX_BUFFER_SIZE];

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------

// -- These are adapted from Adafruit's Adafruit_BluefruitLE_nRF51 library on github.


static uint16_t readline(char * buf, uint16_t bufsize, uint16_t timeout, bool multiline)
{
  uint16_t replyidx = 0;

  while (timeout--) {
	  char c;
    while(0 < g_uartDriver->rx(&c, 1)) {

      if (c == '\r') continue;

      if (c == '\n') {
        // the first '\n' is ignored
        if (replyidx == 0) continue;

        if (!multiline) {
          timeout = 0;
          break;
        }
      }
      buf[replyidx] = c;
      replyidx++;

      // Buffer is full
      if (replyidx >= bufsize) {
        timeout = 0;
        break;
      }
    }

    // delay if needed
    if (timeout) vTaskDelay(pdMS_TO_TICKS(1));
  }

  buf[replyidx] = 0;  // null term

  if (replyidx == 0)
  {
	  DBG_PRINT("[nrf51] Timed out waiting for a response.\n");
  }
  else
  {
	  DBG_PRINT("[nrf51] Received a string from the module: ");
	  DBG_PRINT(buf);
	  DBG_PRINT("\n");
  }

  return replyidx;
}

bool waitForOK(void)
{
  // Use temp buffer to avoid overwrite returned result if any
  char tempbuf[RX_BUFFER_SIZE+1];

  while ( readline(tempbuf, RX_BUFFER_SIZE, DEFAULT_AT_TIMEOUT_ms, false) ) {
    if ( strcmp(tempbuf, "OK") == 0 ) return true;
    if ( strcmp(tempbuf, "ERROR") == 0 ) return false;

    // Copy to internal buffer if not OK or ERROR
    strcpy(g_rxBuffer, tempbuf);
  }
  return false;
}

// -------------------------------------------------------------------------

// Send a basic AT command, waiting for an OK or ERROR.
static bool sendATCommand(const char * cmd)
{
	bool result = false;
    int len = strnlen(cmd, MAX_AT_CMD_LEN);
    if (len == g_uartDriver->tx(cmd, len))
    {
    	DBG_PRINT("[nrf51] Sent the AT command: ");
    	DBG_PRINT(cmd);
    	DBG_PRINT("\n");
    	// Wait for a response.
    	result = waitForOK();
    }

    return result;
}

// Send an AT command, expecting an integer reply.
static bool sendATCommandIntReply(const char * cmd, int32_t * data)
{
	bool result = false;
	int len = strnlen(cmd, MAX_AT_CMD_LEN);
    if (len == g_uartDriver->tx(cmd, len))
    {
    	DBG_PRINT("[nrf51] Sent the AT command: ");
    	DBG_PRINT(cmd);
    	DBG_PRINT("\n");

    	result = waitForOK();
    	if (result)
    	{
    		*data = strtol(g_rxBuffer, NULL, 0);
    	}
    }

    return result;
}

// Send an AT command, expecting a string reply.
static bool sendATCommandStrReply(const char * cmd, char * data, uint32_t numBytes)
{
	bool result = false;
	int len = strnlen(cmd, MAX_AT_CMD_LEN);
    if (len == g_uartDriver->tx(cmd, len))
    {
    	DBG_PRINT("[nrf51] Sent the AT command: ");
    	DBG_PRINT(cmd);
    	DBG_PRINT("\n");

    	// Wait for a response.
    	result = waitForOK();
    	if (result)
    	{
    		strncpy(data, g_rxBuffer, numBytes);
    	}
    }

    return result;
}


int pmBleInit_nRF51(const pmCoreUartDriver_t * driver)
{
	// Pre-condition.
	// - Module is in DATA mode.
	// - Module has echo OFF.

    int rc = PM_BLE_FAILURE;
    if ((NULL != driver) && (NULL != driver->tx) && (NULL != driver->rx))
    {
        g_uartDriver = driver;

        // Check if we are connected to the device.
        if (sendATCommand("AT\n") && sendATCommand("AT+GATTCLEAR\n"))
        {
        	DBG_PRINT("[nrf51] Successfully initialized the nRF51 module.");
        	rc = PM_BLE_SUCCESS;
        }
        else
        {
        	g_uartDriver = NULL;
        }
    }

    return rc;
}

static int pmBleUartTx_nRF51(const uint8_t * bytes, uint8_t numBytes)
{
	int result = 0;
	if (NULL != g_uartDriver)
	{
		(void)snprintf(g_txBuffer, sizeof(g_txBuffer), "AT+BLEUARTTX=%s\n", bytes);
		if (sendATCommand(g_txBuffer))
		{
			result = numBytes;
		}
	}

	return result;
}

static int pmBleUartRx_nRF51(uint8_t * bytes, uint8_t numBytes)
{
	int result = 0;

	if (NULL != g_uartDriver)
	{
		if (sendATCommandStrReply("AT+BLEUARTRX\n", bytes, numBytes) && (bytes[0] != '\0'))
		{
			result = numBytes;
		}
	}
	return result;
}

static int pmBleWriteChar_nRF51(pmBleHandle_t characteristic, pmBleDataType_t data)
{
	int result = PM_BLE_FAILURE;

	if (NULL != g_uartDriver)
	{
		(void)snprintf(g_txBuffer, sizeof(g_txBuffer), "AT+GATTCHAR=%d,%d\n", characteristic, data);

		if (sendATCommand(g_txBuffer))
		{
			result = PM_BLE_SUCCESS;
		}
	}

	return result;
}

static int pmBleReadChar_nRF51(pmBleHandle_t characteristic, pmBleDataType_t * data)
{
	int result = PM_BLE_FAILURE;

	if (NULL != g_uartDriver)
	{
		(void)snprintf(g_txBuffer, sizeof(g_txBuffer), "AT+GATTCHAR=%d\n", characteristic);

		int32_t reply;
		if (sendATCommandIntReply(g_txBuffer, &reply))
		{
			*data = (pmBleDataType_t)reply;
			result = PM_BLE_SUCCESS;
		}
	}

	return result;
}


static int pmBleRegisterService_nRF51(const char * uuid, pmBleCharacteristic_t * charList, uint16_t listLength)
{
	int result = PM_BLE_FAILURE;

	int32_t reply;
	if ((NULL != g_uartDriver) && (NULL != uuid) && (NULL != charList) && (listLength > 0u))
	{
		// Try adding the service.
		int uuidLen = strnlen(uuid, (STRLEN_UUID_128 + 1));

		bool serviceAdded = false;
		if (uuidLen == STRLEN_UUID_16)
		{
			(void)snprintf(g_txBuffer, sizeof(g_txBuffer), "AT+GATTADDSERVICE=UUID=%s\n", uuid);

			serviceAdded = sendATCommandIntReply(g_txBuffer, &reply);
		}
		else if (uuidLen == STRLEN_UUID_128)
		{
			(void)snprintf(g_txBuffer, sizeof(g_txBuffer), "AT+GATTADDSERVICE=UUID128=%s\n", uuid);

			serviceAdded = sendATCommandIntReply(g_txBuffer, &reply);
		}
		else
		{
			// Invalid UUID entered.
			DBG_PRINT("[nrf51] Failed to initialize module due to invalid service UUID: ");
			DBG_PRINT(uuid);
			DBG_PRINT("\n");
		}

		// Now add the characteristics.
		if (serviceAdded)
		{
			result = PM_BLE_SUCCESS; // Assume.
			for (uint16_t ix = 0; (PM_BLE_SUCCESS == result) && (ix < listLength); ix++)
			{
				uuidLen = strnlen(charList[ix].uuid, (STRLEN_UUID_128 + 1));
				if (uuidLen == STRLEN_UUID_16)
				{
					(void)snprintf(
							g_txBuffer,
							sizeof(g_txBuffer),
							"AT+GATTADDCHAR=UUID=%s,PROPERTIES=0x%02X,MIN_LEN=%d,MAX_LEN=%d\n",
							charList[ix].uuid, charList[ix].properties, sizeof(pmBleDataType_t), sizeof(pmBleDataType_t)
					);
				}
				else if (uuidLen == STRLEN_UUID_128)
				{
					(void)snprintf(
							g_txBuffer,
							sizeof(g_txBuffer),
							"AT+GATTADDCHAR=UUID128=%s,PROPERTIES=0x%02X,MIN_LEN=%d,MAX_LEN=%d\n",
							charList[ix].uuid, charList[ix].properties, sizeof(pmBleDataType_t), sizeof(pmBleDataType_t)
					);
				}
				else
				{
					result = PM_BLE_FAILURE;
					DBG_PRINT("[nrf51] Failed to initialize module due to invalid characteristic UUID: ");
					DBG_PRINT(charList[ix].uuid);
					DBG_PRINT("\n");
				}

				if (PM_BLE_FAILURE != result)
				{
					int32_t reply;
					if (sendATCommandIntReply(g_txBuffer, &reply))
					{
						charList[ix].handle = (pmBleHandle_t)reply;
					}
					else
					{
						result = PM_BLE_FAILURE;
					}
				}
			}
		}
	}

	if (PM_BLE_SUCCESS == result)
	{
		// Reset the module for the changes to take effect.
		if (sendATCommand("ATZ\n"))
		{
			vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second for the reset to complete.
			if (!sendATCommand("AT\n"))
			{
				result = PM_BLE_FAILURE;
			}
			else
			{
				DBG_PRINT("[nrf51] Successfully registered the service UUID: ");
				DBG_PRINT(uuid);
				DBG_PRINT("\n");
			}
		}
	}

	return result;
}
