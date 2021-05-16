/*
 * mics_sensor.c
 *
 *  Created on: Apr 25, 2021
 *      Author: Hemant Gopalsing
 */

#include "mics_sensor.h"
#include "stm32f4xx_hal.h"
#include "../sensor_interface.h"
#include "main.h"

int mics_I2C2_transmit(uint8_t dev_id, uint8_t reg_addr, uint8_t * data, uint8_t numBytes)
{
	if (HAL_I2C_Master_Transmit(&hi2c1,dev_id, data, numBytes,100) == HAL_OK)
	{
		return 1;
	}

	return 0;
};

int mics_I2C2_receive(uint8_t dev_id, uint8_t reg_addr, uint8_t * data, uint8_t numBytes)
{
	uint8_t txbuffer[6] = {0};
	txbuffer[0] = 0x0C;
	if (HAL_I2C_Master_Transmit(&hi2c1, 0xE0, txbuffer, numBytes,100) == HAL_OK)
	{
		HAL_Delay(100);
		if (HAL_I2C_Master_Receive(&hi2c1,dev_id, data, numBytes,100) == HAL_OK)
		{
			return 1;
		}
    }

	return 0;
};

mics_driver_inits functions_init[mics_num_interfaces] =
{
		[MICS_I2C2] = {
						.slave_device_id = MICS_PRIAMRY_I2C_ADDRESS,
						.read_from_mics = mics_I2C2_receive,
						.write_to_mics = mics_I2C2_transmit,
					  },
};

uint8_t MICS_VZ_89TE_GetCRC (uint8_t* buffer, uint8_t size)
{
  uint8_t  crc = 0U;
  uint32_t i   = 0U;
  uint32_t sum = 0UL;

  /* Summation with carry   */
  for ( i = 0U; i < size; i++ )
  {
    sum +=   buffer[i];
  }


  crc  = (uint8_t)sum;
  crc += (sum/0x0100);    // Add with carry
  crc  = 0xFF - crc;      // complement


  return  crc;
}


unsigned int getSensorReading (uint8_t * rx_buffer)
{

	//txbuffer[4] = MICS_VZ_89TE_GetCRC(txbuffer,5);


	//mics_I2C2_transmit(0xE0,0,txbuffer,5);
	mics_I2C2_receive(0xE1,0,rx_buffer,7);
	return 1;
}






