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
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <dbg_uart.h>

#include "pm_ble_nrf51.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------
#define MAX_AT_CMD_LEN 64

#define DBG_PRINT(s) dbg_str(s)
//#define DBG_PRINT(s)

// --------------------------------------------------------------------------------------------------------------------
// TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------

static int pmBleUartTx_nRF51(const uint8_t * bytes, uint8_t numBytes);
static int pmBleUartRx_nRF51(uint8_t * bytes, uint8_t numBytes);

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
const pmCoreUartDriver_t pmBleUartService_nRF51 =
{
    .tx = pmBleUartTx_nRF51,
    .rx = pmBleUartRx_nRF51
};

static const pmCoreUartDriver_t * g_uartDriver = NULL;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------

// -- These are adapted from Adafruit's Adafruit_BluefruitLE_nRF51 library on github.
#define BLE_BUFSIZE 64
static uint8_t bleBuffer[BLE_BUFSIZE];

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
    if (timeout) vTaskDelay(1);
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
  char tempbuf[BLE_BUFSIZE+1];

  while ( readline(tempbuf, BLE_BUFSIZE, 100, false) ) {
    if ( strcmp(tempbuf, "OK") == 0 ) return true;
    if ( strcmp(tempbuf, "ERROR") == 0 ) return false;

    // Copy to internal buffer if not OK or ERROR
    strcpy(bleBuffer, tempbuf);
  }
  return false;
}

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


int pmBleInit_nRF51(const pmCoreUartDriver_t * driver)
{
    int rc = PM_BLE_FAILURE;
    if ((NULL != driver) && (NULL != driver->tx) && (NULL != driver->rx))
    {
        g_uartDriver = driver;

        // Check if we are connected to the device.
        if (sendATCommand("AT\n"))
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

static bool dataMode = false;
static int pmBleUartTx_nRF51(const uint8_t * bytes, uint8_t numBytes)
{
	int result = 0;
	if (NULL != g_uartDriver)
	{
		(void)snprintf(bleBuffer, sizeof(bleBuffer), "AT+BLEUARTTX=%s\n", bytes);
		if (sendATCommand(bleBuffer))
		{
			result = numBytes;
		}
	}

	return result;
}

static int pmBleUartRx_nRF51(uint8_t * bytes, uint8_t numBytes)
{
	return 0;
