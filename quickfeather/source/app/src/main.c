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
 *    Purpose: main for QuickFeather helloworldsw and LED/UserButton test
 *                                                          
 *=========================================================*/

#include "Fw_global_config.h"   // This defines application specific charactersitics

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "RtosTask.h"

/*    Include the generic headers required for QORC */
#include "eoss3_hal_gpio.h"
#include "eoss3_hal_rtc.h"
#include "eoss3_hal_fpga_usbserial.h"
#include "eoss3_hal_uart.h"
#include "ql_time.h"
#include "s3x_clock_hal.h"
#include "s3x_clock.h"
#include "s3x_pi.h"
#include "dbg_uart.h"

#include "cli.h"

#include "fpga_loader.h"    // API for loading FPGA
#include "top_bit.h"        // FPGA bitstream to load into FPGA

#include "pm_core.h"

extern const struct cli_cmd_entry my_main_menu[];


const char *SOFTWARE_VERSION_STR;


/*
 * Global variable definition
 */


extern void qf_hardwareSetup();
static void nvic_init(void);

#if 0
static volatile int test = 0;

static void uartFpgaTestTask(void * params)
{
    bool flit = true;
    const char * header = "Hello -- this is a UART FPGA\n";

    uart_tx_buf(UART_ID_FPGA, header, strlen(header));


    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        int numBytes = uart_rx_available(UART_ID_FPGA);
        while (numBytes > 0)
        {
            uart_tx(UART_ID_FPGA, uart_rx(UART_ID_FPGA));
            numBytes--;
        }

        uint8_t usrBtn;
        HAL_GPIO_Read(0, &usrBtn);

        if (!usrBtn)
        {
#define SENDING_UART   UART_ID_FPGA_UART1
#define RECEIVING_UART UART_ID_FPGA

        	// Flush any existing bytes.
        	uint8_t dummy;
        	while (uart_rx_available(RECEIVING_UART) > 0)
			{
				uart_rx_raw_buf(RECEIVING_UART, &dummy, 1);
			}

        	static uint8_t rxbuffer[256];
        	volatile uint8_t errorCount = 0;

			for (uint16_t c = 0; c < sizeof(rxbuffer); c++)
			{
				uart_tx_raw(SENDING_UART, c & 0xFF);

				while (uart_rx_available(RECEIVING_UART) == 0); // Block until data is available.

				uart_rx_raw_buf(RECEIVING_UART, &rxbuffer[c], 1);
			}

			for (uint16_t c = 0; c < sizeof(rxbuffer); c++)
			{
				if (c != rxbuffer[c])
				{
					errorCount++;
				}
			}

			while(1){} // Block here so that the data can be viewed in a debugger.
        }

        flit = !flit;
        //HAL_GPIO_Write(5, flit);
    }
}
#endif

int main(void)
{

    SOFTWARE_VERSION_STR = "polymath-iot/app";
    
    qf_hardwareSetup();
    nvic_init();
    S3x_Clk_Disable(S3X_FB_21_CLK);
    S3x_Clk_Disable(S3X_FB_16_CLK);
    S3x_Clk_Enable(S3X_A1_CLK);
    S3x_Clk_Enable(S3X_CFG_DMA_A1_CLK);
    load_fpga(sizeof(axFPGABitStream), axFPGABitStream);
#if (FEATURE_USBSERIAL == 1)
    // Use 0x6141 as USB serial product ID (USB PID)
    HAL_usbserial_init2(false, false, 0x6141);        // Start USB serial not using interrupts
    for (int i = 0; i != 4000000; i++) ;   // Give it time to enumerate
#endif

#if (FEATURE_FPGA_UART == 1)
    int uart_id;
    UartBaudRateType brate;
    UartHandler uartObj;
    memset( (void *)&(uartObj), 0, sizeof(uartObj) );

    uart_id = UART_ID_FPGA;
    brate = BAUD_115200;
    uartObj.baud = brate;
    uartObj.wl = WORDLEN_8B;
    uartObj.parity = PARITY_NONE;
    uartObj.stop = STOPBITS_1;
    uartObj.mode = TX_RX_MODE;
    uartObj.hwCtrl = HW_FLOW_CTRL_DISABLE;
    uartObj.intrMode = UART_INTR_ENABLE;
    uartHandlerUpdate(uart_id,&uartObj);

    for (volatile int i = 0; i != 4000000; i++) ;   // [TODO] Analyze and remove need for this delay
    uart_init( uart_id, NULL, NULL, &uartObj);

    uint32_t device_id = *(uint32_t *)FPGA_PERIPH_BASE ;

    if (device_id == 0xABCD0002)
    {
      uart_id = UART_ID_FPGA_UART1;
      brate = BAUD_9600;
      uartObj.baud = brate;
      uartObj.wl = WORDLEN_8B;
      uartObj.parity = PARITY_NONE;
      uartObj.stop = STOPBITS_1;
      uartObj.mode = TX_RX_MODE;
      uartObj.hwCtrl = HW_FLOW_CTRL_DISABLE;
      uartObj.intrMode = UART_INTR_ENABLE;
      uartHandlerUpdate(uart_id,&uartObj);

      uart_init( uart_id, NULL, NULL, &uartObj);
    }
#endif

#if 1
    dbg_str("\n\n");
    dbg_str( "##########################\n");
    dbg_str( "Polymath IoT Application\n");
    dbg_str( "SW Version: ");
    dbg_str( SOFTWARE_VERSION_STR );
    dbg_str( "\n" );
    dbg_str( __DATE__ " " __TIME__ "\n" );
    dbg_str( "##########################\n\n");
#endif

    CLI_start_task( my_main_menu );

    #if 0
    if (pdPASS == xTaskCreate(uartFpgaTestTask, "FPGA Task", 1024, NULL, (configMAX_PRIORITIES - 1), NULL))
    {
        //HAL_GPIO_Write(5, true);
    }
    #endif

    pm_main();
    
    /* Start the tasks and timer running. */
    vTaskStartScheduler();
    dbg_str("\n");
      
    while(1);
}

static void nvic_init(void)
 {
    // To initialize system, this interrupt should be triggered at main.
    // So, we will set its priority just before calling vTaskStartScheduler(), not the time of enabling each irq.
    NVIC_SetPriority(Ffe0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(SpiMs_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(CfgDma_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(Uart_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(FbMsg_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
 }    

//needed for startup_EOSS3b.s asm file
void SystemInit(void)
{

}


