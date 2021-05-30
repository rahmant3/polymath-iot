// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   Refer to the header file.
//
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include "ws2812_led.h"

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static int g_dataSentFlag = 0;
static const ws2812Config_t * g_config = NULL;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if (NULL != g_config)
	{
		HAL_TIM_PWM_Stop_DMA(g_config->timModule, g_config->timChannel);
		g_dataSentFlag = 1;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void WS2812_Init(const ws2812Config_t * config)
{
	if (NULL == g_config)
	{
		g_config = config;
	}
}

// --------------------------------------------------------------------------------------------------------------------
// Red 8 bytes | Blue 8 bytes | Green 8 bytes
void WS2812_Send (uint8_t red, uint8_t green, uint8_t blue)
{
// Input clock is 80 MHz
// PWM period is 100 ticks (800 kHz -> 1.25 us)
// The ontime for a data bit of one is 0.85 us (approx 68 ticks)
// The ontime for a data bit of zero is 0.4 us (approx 32 ticks)
//
// 24 bits of data shall be transmitted in this order Blue-Red-Green -- least-significant bit first (i.e. LSB green bits first)
// Following the 24 bits, at least 50 us (approx 40 ticks) of zeros should elapse.
#define NUM_RGB_ENTRIES   24
#define NUM_EMPTY_ENTRIES 80
#define TOTAL_ENTRIES     (NUM_RGB_ENTRIES + NUM_EMPTY_ENTRIES)

	if (NULL != g_config)
	{
		uint32_t color  = (blue << 16) + (red << 8) + green;
		uint16_t pwmData[TOTAL_ENTRIES] = {0};
		for (int i = 0 ; i < NUM_RGB_ENTRIES; i++)
		{
			if (color & (1<< (NUM_RGB_ENTRIES - 1 - i)))
			{
				pwmData[TOTAL_ENTRIES - 1 - i] = 68;
			}
			else
			{
				pwmData[TOTAL_ENTRIES - 1 - i] = 32;
			}
		}

		HAL_TIM_PWM_Start_DMA(g_config->timModule, g_config->timChannel, (uint32_t *)pwmData, TOTAL_ENTRIES);
		while (!g_dataSentFlag){};
		g_dataSentFlag = 0;
	}
}

