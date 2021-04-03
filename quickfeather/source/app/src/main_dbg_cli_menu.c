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

static void pm_send(const struct cli_cmd_entry *pEntry)
{
    char send_string_buf[64];
    memset(send_string_buf, 0, sizeof(send_string_buf));

    CLI_string_buf_getshow( "string to send ", send_string_buf, sizeof(send_string_buf) );
    
    pm_test_send(send_string_buf);
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
    CLI_CMD_SIMPLE( "send", pm_send, "Send user string over the master node." ),
    CLI_CMD_TERMINATE()
};

const struct cli_cmd_entry my_main_menu[] = {
    CLI_CMD_SUBMENU( "diag", qf_diagnostic, "QuickFeather diagnostic commands" ),
    CLI_CMD_SUBMENU( "test", pm_test, "Polymath test commands" ),
    CLI_CMD_TERMINATE()
};
