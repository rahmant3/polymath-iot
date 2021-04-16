/*==========================================================
 * Copyright 2020 QuickLogic Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *==========================================================*/

/*==========================================================
 *
 *    File   : main.c
 *    Purpose: 
 *                                                          
 *=========================================================*/

#include "Fw_global_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eoss3_hal_gpio.h>
#include "cli.h"
#include <stdbool.h>
#include "dbg_uart.h"
#include "eoss3_hal_uart.h"
#include "task.h"

#include "pm_core.h"

static void togglegreenled(const struct cli_cmd_entry *pEntry)
{
    static bool fLit = false;
    (void)pEntry;
    fLit = !fLit;
    HAL_GPIO_Write(5, fLit);
    return;
}

static void toggleredled(const struct cli_cmd_entry *pEntry)
{
    static bool fLit = false;
    (void)pEntry;
    fLit = !fLit;
    HAL_GPIO_Write(6, fLit);
    return;
}

static void toggleblueled(const struct cli_cmd_entry *pEntry)
{
    static bool fLit = false;
    (void)pEntry;
    fLit = !fLit;
    HAL_GPIO_Write(4, fLit);
    return;
}

static void userbutton(const struct cli_cmd_entry *pEntry)
{
    uint8_t ucVal;
    (void)pEntry;
 
    HAL_GPIO_Read(0, &ucVal);
    if (ucVal) {
        CLI_puts("Not pressed");
    } else {
         CLI_puts("Pressed");
    }
    return;
}

#include "pm_protocol.h"

#define ENABLE_SLAVE_LOOPBACK_TEST

extern pmProtocolContext_t g_pmUartMasterContext;

#ifdef ENABLE_SLAVE_LOOPBACK_TEST
extern pmProtocolContext_t g_pmUartSlaveContext;
#endif

static void pm_send_raw(const struct cli_cmd_entry *pEntry)
{
    char send_string_buf[64];
    memset(send_string_buf, 0, sizeof(send_string_buf));

    CLI_string_buf_getshow( "string to send ", send_string_buf, sizeof(send_string_buf) );
    
    pmProtocolRawPacket_t masterTx;
	masterTx.numBytes = strnlen(send_string_buf, PM_MAX_PAYLOAD_BYTES);
	(void)memcpy(masterTx.bytes, send_string_buf, masterTx.numBytes);

	if (PM_PROTOCOL_SUCCESS == pmProtocolSendPacket(&masterTx, &g_pmUartMasterContext))
	{
		dbg_str("Successfully sent the test string on the master node: ");
		dbg_str(send_string_buf);
		dbg_str("\r\n");
	}
	else
	{
		dbg_str("Error: Failed to send the test string on the master node.\r\n");
	}
}

static void pm_receive_raw(const struct cli_cmd_entry *pEntry)
{
    char receive_string_buf[64];
    memset(receive_string_buf, 0, sizeof(receive_string_buf));

    pmProtocolRawPacket_t masterRx;

    uint8_t rc = pmProtocolReadPacket(&masterRx, &g_pmUartMasterContext);
	if (PM_PROTOCOL_RX_TIMEOUT == rc)
	{
		dbg_str("Error: Encountered a timeout on the MASTER node.\r\n");
	}
	else if (PM_PROTOCOL_CHECKSUM_ERROR == rc)
	{
		dbg_str("Error: Encountered a checksum error on the MASTER node.\r\n");
	}
	else if (PM_PROTOCOL_SUCCESS == rc)
	{
		dbg_str("Successfully obtained a packet on the MASTER node: ");
		int len = strnlen(masterRx.bytes, masterRx.numBytes);

		if (len < sizeof(receive_string_buf))
		{
			memcpy(receive_string_buf, masterRx.bytes, strnlen(masterRx.bytes, masterRx.numBytes));
			dbg_str(receive_string_buf);
			dbg_str("\r\n");
		}
	}
}

static void pm_cmd_status(const struct cli_cmd_entry *pEntry)
{
	pmCmdPayloadDefinition_t masterTx;

	masterTx.commandCode = PM_CMD_WRITE_STATUS;
	masterTx.writeStatus.status = PM_RET_SUCCESS;
	if (PM_PROTOCOL_SUCCESS == pmProtocolSend(&masterTx, &g_pmUartMasterContext))
	{
		dbg_str("Successfully sent the status on the master node.\r\n");
	}
	else
	{
		dbg_str("Error: Failed to send the status on the master node.\r\n");
	}
}


static void pm_cmd_protocol(const struct cli_cmd_entry *pEntry)
{
	pmCmdPayloadDefinition_t masterTx;

	masterTx.commandCode = PM_CMD_PROTOCOL_ID;
	if (PM_PROTOCOL_SUCCESS == pmProtocolSend(&masterTx, &g_pmUartMasterContext))
	{
		dbg_str("Successfully sent a request for the protocol ID on the master node.\r\n");
	}
	else
	{
		dbg_str("Error: Failed to send a request for the protocol ID on the master node.\r\n");
	}
}

static void pm_cmd_sensors(const struct cli_cmd_entry *pEntry)
{
	pmCmdPayloadDefinition_t masterTx;

	masterTx.commandCode = PM_CMD_GET_SENSORS;
	if (PM_PROTOCOL_SUCCESS == pmProtocolSend(&masterTx, &g_pmUartMasterContext))
	{
		dbg_str("Successfully sent a request for the sensors on the master node.\r\n");
	}
	else
	{
		dbg_str("Error: Failed to send a request for the sensors on the master node.\r\n");
	}
}


static void pm_cmd_rx(const struct cli_cmd_entry *pEntry)
{
	pmCmdPayloadDefinition_t masterRx;

	if (PM_PROTOCOL_SUCCESS == pmProtocolRead(&masterRx, &g_pmUartMasterContext))
	{
		dbg_str("Successfully read a payload on the master node.\r\n");
		switch (masterRx.commandCode & 0x7F)
		{
			case PM_CMD_NEGATIVE_ACK:
				dbg_str("Received a negative ACK.\r\n");
				break;
			case PM_CMD_PROTOCOL_ID:
				dbg_str("Receive a protocol ID of: ");
				dbg_hex32(masterRx.protocolInfo.protocolIdentifier);
				dbg_str("\r\n");
				break;
			case PM_CMD_WRITE_STATUS:
				dbg_str("Receive a write status of: ");
				dbg_int(masterRx.writeStatus.status);
				dbg_str("\r\n");
				break;
			case PM_CMD_GET_SENSORS:
				dbg_str("Received sensor data:\r\n");
				for (int ix = 0; ix < masterRx.getSensors.numSensors; ix++)
				{
					dbg_str("\tSensor ID: ");
					dbg_int(masterRx.getSensors.sensors[ix].sensorId);
					dbg_str("\t Data: ");
					dbg_int(masterRx.getSensors.sensors[ix].data);
					dbg_str("\r\n");
				}
				dbg_str("\r\n");
				break;
			default:
				dbg_str("Received unhandled command.\r\n");
				break;
		}
	}
	else
	{
		dbg_str("Error: Failed to read a payload on the master node.\r\n");
	}
}


static void pm_slave_receive_raw(const struct cli_cmd_entry *pEntry)
{
    // Check for reads on the slave node.
    pmProtocolRawPacket_t slaveRx;

    uint8_t rc = pmProtocolReadPacket(&slaveRx, &g_pmUartSlaveContext);
    if (PM_PROTOCOL_RX_TIMEOUT == rc)
    {
        dbg_str("Error: Encountered a timeout on the slave node.\r\n");
    }
    else if (PM_PROTOCOL_CHECKSUM_ERROR == rc)
    {
        dbg_str("Error: Encountered a checksum error on the slave node.\r\n");
    }
    else if (PM_PROTOCOL_SUCCESS == rc)
    {
        dbg_str("Successfully obtained a packet on the slave node. Echoing.\r\n");
        if (PM_PROTOCOL_SUCCESS == pmProtocolSendPacket(&slaveRx, &g_pmUartSlaveContext))
        {
            dbg_str("Successfully echo'd the packet.\r\n");
        }
        else
        {
        	dbg_str("Error: Failed to transmit echoed packet.\r\n");
        }
    }
    else
    {
    	dbg_str("No packet received.\r\n");
    }
}


static void pm_slave_receive_cmd(const struct cli_cmd_entry *pEntry)
{
    // Check for reads on the slave node.
	pmCmdPayloadDefinition_t slaveRx;

	if (PM_PROTOCOL_SUCCESS == pmProtocolRead(&slaveRx, &g_pmUartSlaveContext))
	{
		dbg_str("Successfully read a payload on the slave node. Responding\r\n");
		switch (slaveRx.commandCode & 0x7F)
		{
			case PM_CMD_PROTOCOL_ID:
			{
				// Transmit the current protocol ID.
				pmCmdPayloadDefinition_t slaveTx;
				slaveTx.commandCode = slaveRx.commandCode | PM_PROTOCOL_RESP_MASK;
				slaveTx.protocolInfo.protocolIdentifier = PM_PROTOCOL_IDENTIFIER;

				if (PM_PROTOCOL_SUCCESS != pmProtocolSend(&slaveTx, &g_pmUartSlaveContext))
				{
					dbg_str("Error: Failed to transmit response from slave.\r\n");
				}
				break;
			}
			case PM_CMD_WRITE_STATUS:
			{
				// Echo back the status.
				pmCmdPayloadDefinition_t slaveTx;
				slaveTx.commandCode = slaveRx.commandCode | PM_PROTOCOL_RESP_MASK;
				slaveTx.writeStatus.status = slaveRx.writeStatus.status;

				if (PM_PROTOCOL_SUCCESS != pmProtocolSend(&slaveTx, &g_pmUartSlaveContext))
				{
					dbg_str("Error: Failed to transmit response from slave.\r\n");
				}
				break;
			}
			case PM_CMD_GET_SENSORS:
			{
				// Respond with some dummy data.
				pmCmdPayloadDefinition_t slaveTx;
				slaveTx.commandCode = slaveRx.commandCode | PM_PROTOCOL_RESP_MASK;
				slaveTx.getSensors.numSensors = 5;
				for (int ix = 0; ix < slaveTx.getSensors.numSensors; ix++)
				{
					slaveTx.getSensors.sensors[ix].sensorId = ix;
					slaveTx.getSensors.sensors[ix].data = ix + 0x80;
				}

				if (PM_PROTOCOL_SUCCESS != pmProtocolSend(&slaveTx, &g_pmUartSlaveContext))
				{
					dbg_str("Error: Failed to transmit response from slave.\r\n");
				}
				break;
			}
			default:
				dbg_str("Received unhandled command on the slave.\r\n");
				break;
		}
	}
	else
	{
		dbg_str("Error: Failed to read a payload on the master node.\r\n");
	}
}


const struct cli_cmd_entry qf_diagnostic[] =
{
    CLI_CMD_SIMPLE( "red", toggleredled, "toggle red led" ),
    CLI_CMD_SIMPLE( "green", togglegreenled, "toggle green led" ),
    CLI_CMD_SIMPLE( "blue", toggleblueled, "toggle blue led" ),
    CLI_CMD_SIMPLE( "userbutton", userbutton, "show state of user button" ),
    CLI_CMD_TERMINATE()
};

const struct cli_cmd_entry pm_test[] =
{
    CLI_CMD_SIMPLE( "m_send_raw",    pm_send_raw,    "Send user string over the master node." ),
	CLI_CMD_SIMPLE( "m_receive_raw", pm_receive_raw, "Read a user string on the master node." ),
	CLI_CMD_SIMPLE( "m_receive_cmd", pm_cmd_rx,      "Read a response command on the master node." ),

	CLI_CMD_SIMPLE( "m_send_cmd_0", pm_cmd_protocol, "Request protocol ID from the slave node." ),
	//CLI_CMD_SIMPLE( "m_send_cmd_1", pm_cmd_, "Request cluster ID from the slave node." ),
	CLI_CMD_SIMPLE( "m_send_cmd_2", pm_cmd_sensors, "Request sensor data from the slave node." ),
	CLI_CMD_SIMPLE( "m_send_cmd_3", pm_cmd_status, "Send a status to the slave node." ),


	CLI_CMD_SIMPLE( "s_receive_raw", pm_slave_receive_raw, "Read user string from the slave node." ),
	CLI_CMD_SIMPLE( "s_receive_cmd", pm_slave_receive_cmd, "Read command from the slave node." ),
    CLI_CMD_TERMINATE()
};

const struct cli_cmd_entry my_main_menu[] = {
    CLI_CMD_SUBMENU( "diag", qf_diagnostic, "QuickFeather diagnostic commands" ),
    CLI_CMD_SUBMENU( "test", pm_test, "Polymath test commands" ),
    CLI_CMD_TERMINATE()
};
