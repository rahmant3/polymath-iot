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

/*
 * Accel_BMI160.c
 *
 */

#include "Fw_global_config.h"
#include <FreeRTOS.h>
#include <eoss3_hal_i2c.h>
#include <task.h>
#include <timers.h>
#include "QL_Trace.h"
#include "QL_SDF.h"
#include "QL_SensorIoctl.h"
#include "ffe_ipc_lib.h"
#include "QL_FFE_SensorConfig.h"
//#include "ql_testTask.h"
#include "QL_SensorCommon.h"
#include "QL_SDF_Accel.h"

#ifdef PURE_M4_DRIVERS


I2C_Config xI2CConfig;
static TimerHandle_t AccelBtimer=NULL;

static QL_Accel_Ffe_Drv accel_drv_priv;

//static uint8_t AccelAxisDir;
static uint8_t AccelAxisRange;
static unsigned int AccelODR;
static uint8_t IsAccelEnabled;
static unsigned int batchPeriod;
static unsigned int batchcount,samplescnt,sensorid;


static Accel_data	Buf_Bdata[100]; //for Batch Data

static Accel_data *Bufp = &Buf_Bdata[0];

static struct QL_SF_Ioctl_Req_Events AccelCBinfo[QL_FFE_MAX_SENSORS];
static struct QL_SensorEventInfo Batchevent;

#define BMI160_DEV_I2C_ADDR         0x68
#define BMI160_DEV_I2C_BUS			0x0

#define BMI160_CHIPID_REG      		0x00 //Chip ID Register
#define BMI160_ERR_STATUS_REG     	0x02 //Error Status Register
#define BMI160_PMU_STATUS_REG  		0x03 //Power Management Unit Status Register
#define BMI160_STATUS_REG      		0x1B //Status Register

#define BMI160_CMD_REG             	0x7E //CMD Register
#define BMI160_ACC_CONF_REG        	0x40 //Accel configuration
#define BMI160_ACC_PMU_RANGE_REG   	0x41 //Accel Range setup
#define BMI160_ACC_X_Reg           	0x12 //Start of X,Y,Z values = LSB of X value


#define BMI160_CHIP_ID_VALUE        				0xD1
#define BMI160_SOFT_RESET_VALUE						0xB6
#define BMI160_ERR_STATUS_NO_ERROR					0x00
#define BMI160_ODR_VALUE_50HZ						0x27
#define BMI160_RANGE_VALUE_4G						0x05
#define BMI160_CMD_VALUE_NORMALPOWERMODE			0x11
#define BMI160_CMD_VALUE_LOWPOWERMODE				0x12
#define BMI160_CMD_VALUE_SUSPENDMODE				0x13
#define BMI160_PMU_STATUS_VALUE_LOWPOWERMODE		0x20
#define BMI160_PMU_STATUS_VALUE_NORMALPOWERMODE		0x10

//Note: These are Register values to be Programmed
//Note: These are also used as indices to lookup Sample Periods
#define BMI160_ACCEL_ODR_RATE_25by32_HZ     1  // 25/32 Hz
#define BMI160_ACCEL_ODR_RATE_25by16_HZ     2  // 25/16 Hz
#define BMI160_ACCEL_ODR_RATE_25by8_HZ      3  // 25/8 Hz
#define BMI160_ACCEL_ODR_RATE_25by4_HZ      4  // 25/4 Hz
#define BMI160_ACCEL_ODR_RATE_25by2_HZ      5  // 25/2 Hz
#define BMI160_ACCEL_ODR_RATE_25_HZ         6  // 25 Hz
#define BMI160_ACCEL_ODR_RATE_50_HZ         7  // 50 Hz
#define BMI160_ACCEL_ODR_RATE_100_HZ        8  // 100 Hz
#define BMI160_ACCEL_ODR_RATE_200_HZ        9  // 200 Hz
#define BMI160_ACCEL_ODR_RATE_400_HZ        10  // 400 Hz
#define BMI160_ACCEL_ODR_RATE_800_HZ        11  // 800 Hz
#define BMI160_ACCEL_ODR_RATE_1600_HZ       12  // 1600 Hz

#define BMI160_DEFAULT_ACCEL_SAMPLE_RATE    BMI160_ACCEL_ODR_RATE_50_HZ //default rate is 50 Hz

//Frequency in Hz. Frequency settings set by M4.
#define ACCEL_RATE_12HZ    12
#define ACCEL_RATE_25HZ    25
#define ACCEL_RATE_50HZ    50
#define ACCEL_RATE_100HZ   100
#define ACCEL_RATE_200HZ   200
#define ACCEL_RATE_400HZ   400
#define ACCEL_RATE_800HZ   800
#define ACCEL_RATE_1600HZ 1600

int convert_m4_freq_to_accel_freq(int freq)
{
	if (ACCEL_RATE_1600HZ == freq)
		return BMI160_ACCEL_ODR_RATE_1600_HZ; //1600Hz
	if (ACCEL_RATE_800HZ < freq)
		return BMI160_ACCEL_ODR_RATE_1600_HZ; //1600Hz
	if (ACCEL_RATE_400HZ < freq)
		return BMI160_ACCEL_ODR_RATE_800_HZ; //800Hz
	if (ACCEL_RATE_200HZ < freq)
		return BMI160_ACCEL_ODR_RATE_400_HZ; //400Hz
	if (ACCEL_RATE_100HZ < freq)
		return BMI160_ACCEL_ODR_RATE_200_HZ; //200Hz
	if (ACCEL_RATE_50HZ < freq)
		return BMI160_ACCEL_ODR_RATE_100_HZ; //100Hz
	if (ACCEL_RATE_25HZ < freq)
		return BMI160_ACCEL_ODR_RATE_50_HZ; //50Hz
	if (ACCEL_RATE_12HZ < freq)
		return BMI160_ACCEL_ODR_RATE_25_HZ; //25Hz
	if (ACCEL_RATE_12HZ == freq)
		return BMI160_ACCEL_ODR_RATE_25by2_HZ; //12.5Hz

	return BMI160_ACCEL_ODR_RATE_100_HZ;	/*ACCEL_RATE_100HZ*/
}


int convert_accel_freq_to_m4_freq(int freq)
{
	if (BMI160_ACCEL_ODR_RATE_1600_HZ == freq)
		return ACCEL_RATE_1600HZ; //1600Hz
	if (BMI160_ACCEL_ODR_RATE_800_HZ == freq)
		return ACCEL_RATE_800HZ; //1600Hz
	if (BMI160_ACCEL_ODR_RATE_400_HZ == freq)
		return ACCEL_RATE_400HZ; //800Hz
	if (BMI160_ACCEL_ODR_RATE_200_HZ == freq)
		return ACCEL_RATE_200HZ; //400Hz
	if (BMI160_ACCEL_ODR_RATE_100_HZ == freq)
		return ACCEL_RATE_100HZ; //200Hz
	if (BMI160_ACCEL_ODR_RATE_50_HZ== freq)
		return ACCEL_RATE_50HZ; //100Hz
	if (BMI160_ACCEL_ODR_RATE_25_HZ == freq)
		return ACCEL_RATE_25HZ; //50Hz


	return BMI160_ACCEL_ODR_RATE_100_HZ;	/*ACCEL_RATE_100HZ*/
}


// 0 maps to +-2G, 1 maps to +-4G, 2 maps to +-8G, 3 maps to +-16G
int convert_m4_range_to_accel_range(int range){
	switch(range){
	case 0:
		return 0x3; // 0b0011
	case 1:
		return 0x5; // 0b0101
	case 2:
		return 0x8; // 0b1000
	case 3:
		return 0xC; // 0b1100
	default:
		return 0x3; // 0b0011
	}
}

int convert_accel_range_to_m4_range(int range){
	switch(range){
	case 0x3:
		return 0; // 0b0011
	case 0x5:
		return 1; // 0b0101
	case 0x8:
		return 2; // 0b1000
	case 0xC:
		return 3; // 0b1100
	default:
		return 0; // 0b0011
	}
}

double Get_Accel_Scale_Factor(void){
	//double scale;
	//scale = (double)((double)AccelAxisRange / 32767.0) * (9.8);
	switch(AccelAxisRange) {
	case 0:
		return 0.000598163;
	case 1:
		return 0.001196326;
	case 2:
		return 0.002392651;
	case 3:
		return 0.004785302;
	}
	return 0;
}

float Get_Accel_Scale_Factor_1000mG(){
	float scale;
	scale = (1000.0 / 32767) * ((float)AccelAxisRange);
	return scale;
}

static QL_Status Read_AccelData(uint8_t reg, void *buf,int length){
	Accel_data Adata;
	uint8_t AccelData[6];
	uint8_t *tbuf = AccelData;
	Accel_data *pbuf = (Accel_data *)buf;

	for(int i=0;i< length; i++){
		if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, reg++, tbuf++, 1) != HAL_OK) return QL_STATUS_ERROR;
	}

	Adata.dataX = 0xFFFF & ( AccelData[1] << 8 | AccelData[0] << 0);
	Adata.dataY = 0xFFFF & ( AccelData[3] << 8 | AccelData[2] << 0);
	Adata.dataZ = 0xFFFF & ( AccelData[5] << 8 | AccelData[4] << 0);

	//memcpy(pbuf,&Adata,sizeof(Adata));
	pbuf->dataX = Adata.dataX;
	pbuf->dataY = Adata.dataY;
	pbuf->dataZ = Adata.dataZ;

#if 0
	switch(0) {
	case 0: //XYZ
		pbuf->dataX = Adata.dataX;
		pbuf->dataY = Adata.dataY;
		pbuf->dataZ = Adata.dataZ;
		break;
	case 1: //Y-XZ
		pbuf->dataX = Adata.dataY;
		pbuf->dataY = -Adata.dataX;
		pbuf->dataZ = Adata.dataZ;
		break;
	case 2: //-X-YZ
		pbuf->dataX = -Adata.dataY;
		pbuf->dataY = -Adata.dataX;
		pbuf->dataZ = Adata.dataZ;
		break;
	case 3: // -YXZ
		pbuf->dataX = -Adata.dataY;
		pbuf->dataY = Adata.dataX;
		pbuf->dataZ = Adata.dataZ;
		break;
	case 4: //-XY-Z
		pbuf->dataX = -Adata.dataX;
		pbuf->dataY = Adata.dataY;
		pbuf->dataZ = -Adata.dataZ;
		break;
	case 5: //YX-Z
		pbuf->dataX = Adata.dataY;
		pbuf->dataY = Adata.dataX;
		pbuf->dataZ = -Adata.dataZ;
		break;
	case 6: //X-Y-Z
		pbuf->dataX = Adata.dataX;
		pbuf->dataY = -Adata.dataY;
		pbuf->dataZ = -Adata.dataZ;
		break;
	case 7: //-Y-X-Z
		pbuf->dataX = -Adata.dataY;
		pbuf->dataY = -Adata.dataX;
		pbuf->dataZ = -Adata.dataZ;
		break;
	}
#endif
	return QL_STATUS_OK;
}

void vAccelBtimerCB( TimerHandle_t xATimer ) {

	Read_AccelData(BMI160_ACC_X_Reg,Bufp,6);
	//printf(" vAccelBtimerCB : Accel data read X=%hd,Y=%hd,Z=%hd\n",Bufp->dataX, Bufp->dataY, Bufp->dataZ);
	Bufp++;
	samplescnt++;
	batchcount--;

	if(batchcount == 0) {
		if(xTimerChangePeriod(AccelBtimer,pdMS_TO_TICKS(1000),pdMS_TO_TICKS(10)) == pdPASS);
		if(xTimerStop(AccelBtimer,pdMS_TO_TICKS(10)) == pdPASS);

		if(AccelCBinfo[sensorid].event_callback !=NULL) {
			Batchevent.event = QL_SENSOR_EVENT_BATCH;
			Batchevent.numpkt = samplescnt;
			Batchevent.data = &Buf_Bdata[0];
			AccelCBinfo[sensorid].event_callback(AccelCBinfo[sensorid].cookie,&Batchevent);
			//Reset Buf pointer
			Bufp =&Buf_Bdata[0];
			samplescnt=0;
		}
	}
}

static QL_Status M4_BMI160_Accel_open(struct QL_SDF_SensorDrv *s, QL_SDF_SensorDrvInstanceHandle_t *instance_handle) {

	unsigned int index ;

	*instance_handle = (QL_SDF_SensorDrvInstanceHandle_t)0xFFFF;
	QL_ASSERT(s != NULL);
	QL_ASSERT(s == (struct QL_SDF_SensorDrv *) &accel_drv_priv); // we should get back the same  that we provided during registration

	for ( index = 0 ; index < CONFIG_MAX_FFE_ACCEL_INSTANCE; index++ )
	{
//		printf("instance index: %d, state: %d\n",index, accel_drv_priv.devInstances[index].state);
		/* if we have an instance slot with state == SENSOR_STATE_PROBED, this is ready to use */
		if (SENSOR_STATE_PROBED == accel_drv_priv.devInstances[index].state)
		{
			/* move this instance's state to SENSOR_STATE_OPENED to mark that this slot is now in use */
			accel_drv_priv.devInstances[index].state = SENSOR_STATE_OPENED;

			/* pass back the index of the instance in our private table to Framework, as our "Instance Handle" */
			*instance_handle = (QL_SDF_SensorDrvInstanceHandle_t) index;

			/* indicate to framework that open succeeded */
			return QL_STATUS_OK;
		}
	}

	/* if we reach here, we do not have any free slots for further instances, indicate to framework that open failed */
	return QL_STATUS_ERROR;
}

static QL_Status M4_BMI160_Accel_close(QL_SDF_SensorDrvInstanceHandle_t instance_handle) {
	/* KK: check validity of handle. This is probably not needed and can be removed */
	QL_ASSERT((unsigned int) instance_handle < CONFIG_MAX_FFE_ACCEL_INSTANCE);

	/* move the state of this instance back to ready to use, SENSOR_STATE_PROBED */
	accel_drv_priv.devInstances[(unsigned int)instance_handle].state = SENSOR_STATE_PROBED;

	/* indicate to framework that the instance was closed successfully */
	return QL_STATUS_OK;
}

static QL_Status M4_BMI160_Accel_read(QL_SDF_SensorDrvInstanceHandle_t instance_handle, void *buf, int size) {
	uint8_t reg;
	QL_Status status = QL_STATUS_OK;

	QL_ASSERT( (unsigned int)instance_handle < CONFIG_MAX_FFE_ACCEL_INSTANCE);

//	printf("sensorid: %u,cmd: %u\r\n", sensorid, cmd);

	/* check if the instance is in open state first */
	if (SENSOR_STATE_OPENED != accel_drv_priv.devInstances[(unsigned int) instance_handle].state)
	{
		return QL_STATUS_ERROR;
	}

	if(buf == NULL){
		printf("%s - buffer is NULL\n",__func__);
		return QL_STATUS_ERROR;
	}
	reg = BMI160_ACC_X_Reg;
	status = Read_AccelData( reg, buf, size);
	return status;
}


static QL_Status QL_M4_OperationForAccel(unsigned int sensorID, unsigned int operation, void *arg) {

	uint8_t i2c_read_data = 0x0;
	uint8_t i2c_write_data = 0x0;
	unsigned int argval=0;

	switch(operation) {
	case QL_SAL_IOCTL_SET_CALLBACK:
	{
		accel_drv_priv.drvData.sensor_callback = (QL_SensorEventCallback_t)arg;
	}
	break;

	case QL_SAL_IOCTL_GET_CALLBACK:
	{
		*(QL_SensorEventCallback_t *)arg = accel_drv_priv.drvData.sensor_callback;
	}
	break;

	case QL_SAL_IOCTL_SET_DYNAMIC_RANGE:
	{
		AccelAxisRange = *(unsigned int *)arg;
		argval = convert_m4_range_to_accel_range(AccelAxisRange);
		i2c_write_data = argval & 0x0F;
		if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_ACC_PMU_RANGE_REG, &i2c_write_data, 1) != HAL_OK) return QL_STATUS_ERROR;
		break;
	}

	case QL_SAL_IOCTL_GET_DYNAMIC_RANGE:
	{
		if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_ACC_PMU_RANGE_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
		*(unsigned int *)arg = convert_accel_range_to_m4_range((unsigned int) (i2c_read_data & 0x0F));
		break;
	}

	case QL_SAL_IOCTL_SET_ODR:
	{
		AccelODR = *(unsigned int *)arg;
		argval = convert_m4_freq_to_accel_freq(AccelODR);
		/* init sequence: set ODR, range and power mode */
		if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_ACC_CONF_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
		i2c_write_data =  0x20 | argval;//i2c_read_data |(argval & 0x0F);
		if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_ACC_CONF_REG, &i2c_write_data, 1) != HAL_OK) return QL_STATUS_ERROR;
		break;
	}

	case QL_SAL_IOCTL_GET_ODR:
	{
		/* init sequence: set ODR, range and power mode */
		if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_ACC_CONF_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
		*(unsigned int *)arg = convert_accel_freq_to_m4_freq((unsigned int) (i2c_read_data) & ~(0x20));
		break;
	}

	case QL_SAL_IOCTL_SET_BATCH:
	{
		batchPeriod = *(unsigned int *)arg;

		unsigned int sample = (unsigned int)(((float) (1.0/(float)AccelODR)) * 1000);
		sample = sample > 0 ? sample :1;

		if(batchPeriod % sample != 0)
			batchcount = (batchPeriod / sample) + 1;
		else
			batchcount = (batchPeriod / sample);
		sensorid = sensorID;
		vTaskDelay(pdMS_TO_TICKS(100));
		xTimerChangePeriod(AccelBtimer,pdMS_TO_TICKS(sample),portMAX_DELAY);
		xTimerStart(AccelBtimer,portMAX_DELAY);
		break;
	}

	case QL_SAL_IOCTL_GET_BATCH:
	{
		*(unsigned int *)arg = (unsigned int) batchPeriod;
		break;
	}

	case QL_SAL_IOCTL_BATCH_FLUSH:
	{
		if(samplescnt != 0) {
			//STOP the Timer
			if(xTimerChangePeriod(AccelBtimer,pdMS_TO_TICKS(1000),pdMS_TO_TICKS(10)) == pdPASS);
			if(xTimerStop(AccelBtimer,pdMS_TO_TICKS(10)) == pdPASS);

			// Send the event Call back
			if(AccelCBinfo[sensorid].event_callback !=NULL) {
				Batchevent.event = QL_SENSOR_EVENT_BATCH;
				Batchevent.numpkt = samplescnt;
				Batchevent.data = &Buf_Bdata[0];
				AccelCBinfo[sensorid].event_callback(AccelCBinfo[sensorid].cookie,&Batchevent);
				//Reset Buf pointer
				Bufp =&Buf_Bdata[0];
				samplescnt=0;
				batchcount=0;
			}
		}
		break;
	}

	case QL_SAL_IOCTL_ALLOC_DATA_BUF:
	{
		// call the platform function

		if (QL_STATUS_OK != QL_SDF_SensorAllocMem(	sensorID,
				(void **)&(((struct QL_SF_Ioctl_Set_Data_Buf*)arg)->buf) ,
				((struct QL_SF_Ioctl_Set_Data_Buf*)arg)->size) )
		{
			QL_TRACE_SENSOR_ERROR("QL_SDF_SensorAllocMem failed to get memory for FFE sensor ID = %d\n", sensorID);
			return	QL_STATUS_ERROR;
		}
	}
	break;

	case QL_SAL_IOCTL_SET_DATA_BUF:
	{
		// not implemented
		break;
	}
	case QL_SAL_IOCTL_GET_DATA_BUF:
	{
		// not implemented
		break;
	}

	case QL_SAL_IOCTL_LIVE_DATA_ENABLE:
	{
		// not implemented
	}
	break;

	case QL_SAL_IOCTL_GET_LIVE_DATA_ENABLE:
	{
		// not implemented
	}
	break;

	case QL_SAL_IOCTL_ENABLE:
	{
		//Start the Accelerometer.
		IsAccelEnabled = (uint8_t) (*(unsigned int*)arg);
		break;
	}

	case QL_SAL_IOCTL_GET_ENABLE:
	{
		*(unsigned int *)arg = (unsigned int)IsAccelEnabled;
		break;
	}

	default:
		QL_TRACE_SENSOR_ERROR(" Operation - %d , Failed. Sensor ID %d\n",operation,sensorID);
		break;
	}

	return QL_STATUS_OK;
}

static QL_Status M4_BMI160_Accel_ioctl(QL_SDF_SensorDrvInstanceHandle_t instance_handle, unsigned int cmd, void *arg) {
	unsigned int sensorid;

	sensorid = accel_drv_priv.ffe_accel_id;
	QL_ASSERT( (unsigned int)instance_handle < CONFIG_MAX_FFE_ACCEL_INSTANCE);

//	printf("sensorid: %u,cmd: %u\r\n", sensorid, cmd);

	/* check if the instance is in open state first */
	if (SENSOR_STATE_OPENED != accel_drv_priv.devInstances[(unsigned int) instance_handle].state)
	{
		return QL_STATUS_ERROR;
	}

	/* before invoking the command implementation, ensure that the arguments are valid */
	/* the arg check function *must* be specific to the end sensor being used and has to be checked ! */
	//TODO: Check for the Adaptability
//	if(QL_STATUS_OK != Accel_FFE_IsArgValid(cmd, arg))
//	{
//		QL_TRACE_SENSOR_ERROR("Command %d Arguments Invalid for sensor ID = %d\n", cmd, sensorid);
//		return QL_STATUS_ERROR;
//	}

	return QL_M4_OperationForAccel(sensorid, cmd, arg);
}

QL_Status M4_BMI160_Accel_Init(void)
{
	/*
	 * BMI 160 ACCEL Init sequence
	 *
	 * (1) Read CHIP ID and verify (CHIP ID REG)
	 * (2) Soft Reset ???
	 * (3) Wait for 50 ms after Soft Reset, Check if any error (ERROR STATUS REG)
	 * (4) Set default Rate(ODR), range and power mode(LP/HP)
	 * (5) Wait atleast 5ms, Check if any error (ERROR STATUS REG) and power mode (PMU STATUS REG) is as set
	 *
	 */

	uint8_t i2c_read_data = 0x0;
	uint8_t i2c_write_data = 0x0;

	xI2CConfig.eI2CFreq = I2C_200KHZ;
	xI2CConfig.eI2CInt = I2C_DISABLE;
	xI2CConfig.ucI2Cn = BMI160_DEV_I2C_BUS;					/* SM0 == I2C0 */

	HAL_I2C_Init(xI2CConfig);

	/* chip id check */
	if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_CHIPID_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != BMI160_CHIP_ID_VALUE) return QL_STATUS_ERROR;

	printf("BMI160 chip id check OK (chip id = 0x%x)\n", i2c_read_data);

	/* soft reset */
	i2c_write_data = BMI160_SOFT_RESET_VALUE;
	if( HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_CMD_REG, &i2c_write_data, 1) != HAL_OK ) return QL_STATUS_ERROR;

	/* wait for 50ms */
	volatile int x = 50000;
	while(x--);
	//vTaskDelay(pdMS_TO_TICKS(50));

	/* check for any error */
	if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_ERR_STATUS_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != BMI160_ERR_STATUS_NO_ERROR) return QL_STATUS_ERROR;

	printf("BMI160 Soft Reset OK, no error after reset\n");

	/* init sequence: set ODR, range and power mode */
	i2c_write_data = BMI160_ODR_VALUE_50HZ;
	if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_ACC_CONF_REG, &i2c_write_data, 1) != HAL_OK) return QL_STATUS_ERROR;

	i2c_write_data = BMI160_RANGE_VALUE_4G;
	if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_ACC_PMU_RANGE_REG, &i2c_write_data, 1) != HAL_OK) return QL_STATUS_ERROR;

	i2c_write_data = BMI160_CMD_VALUE_NORMALPOWERMODE;
	if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_CMD_REG, &i2c_write_data, 1) != HAL_OK) return QL_STATUS_ERROR;

	printf("BMI160 Write ODR, Range, Power Mode OK\n");

	/* wait for atleast 5 ms */
	volatile int z = 5000;
	while(z--);
	//vTaskDelay(pdMS_TO_TICKS(5));

	/* check error status */
	if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_ERR_STATUS_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != BMI160_ERR_STATUS_NO_ERROR)   return QL_STATUS_ERROR;

	printf("BMI160 Init Sequence OK, no errors\n");

	/* check PMU status */
	if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_PMU_STATUS_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != BMI160_PMU_STATUS_VALUE_NORMALPOWERMODE) return QL_STATUS_ERROR;

	printf("BMI160 PMU Status OK, power mode as expected (0x%x)\n", i2c_read_data);

	/* at this point probe() is ok */
	printf("BMI160 Probe on M4 OK\n\n");

	return QL_STATUS_OK;
}

QL_Status M4_BMI160_Accel_probe(struct QL_SDF_SensorDrv *s)
{
	unsigned int index;

	QL_ASSERT(s != NULL);
	QL_ASSERT(s == (struct QL_SDF_SensorDrv *)&accel_drv_priv); // we should get back the same instance that we provided during registration

	if(QL_STATUS_OK != M4_BMI160_Accel_Init())
	{
		QL_TRACE_SENSOR_ERROR("M4_BMI160_Accel_probe() failed for SAL sensor with ID = %d\n",accel_drv_priv.drvData.id);

		/* probe failed, update the state of each of the instances to not ready, SENSOR_STATE_INVALID */
		for ( index = 0 ; index < CONFIG_MAX_FFE_ACCEL_INSTANCE; index++ )
		{
			accel_drv_priv.devInstances[index].state = SENSOR_STATE_INVALID;
		}

		return QL_STATUS_ERROR;
	}
	QL_TRACE_SENSOR_DEBUG("BMI160_Accel_Probe_In_M4() OK for SAL sensor with ID = %d\n",accel_drv_priv.drvData.id);

	/* probe is ok, update the state of each of the instances to ready to use, SENSOR_STATE_PROBED */
	for ( index = 0 ; index < CONFIG_MAX_FFE_ACCEL_INSTANCE; index++ )
	{
		accel_drv_priv.devInstances[index].state = SENSOR_STATE_PROBED;
	}

	return QL_STATUS_OK;
}

static QL_Status M4_BMI160_Accel_suspend(QL_SDF_SensorDrvInstanceHandle_t instance_handle) {

	uint8_t cmd;
	cmd = BMI160_CMD_VALUE_LOWPOWERMODE;
	// Write the command register for Accel Low power mode
	if(HAL_I2C_Write((UINT8_t)BMI160_DEV_I2C_ADDR,(UINT8_t)BMI160_CMD_REG,(UINT8_t*)cmd,1) != HAL_OK) {
		return QL_STATUS_ERROR;
	}
	// Wait for 4MS
	vTaskDelay(pdMS_TO_TICKS(4));

	return QL_STATUS_OK;
}


static QL_Status M4_BMI160_Accel_resume(QL_SDF_SensorDrvInstanceHandle_t instance_handle) {

	uint8_t i2c_read_data,cmd;

	cmd = BMI160_CMD_VALUE_NORMALPOWERMODE;
	// Write the command register for Accel Low power mode
	if(HAL_I2C_Write(BMI160_DEV_I2C_ADDR, BMI160_CMD_REG, &cmd,1) != HAL_OK) return QL_STATUS_ERROR;
	// Wait for 4MS
	vTaskDelay(pdMS_TO_TICKS(4));

	/* check PMU status */
	if(HAL_I2C_Read(BMI160_DEV_I2C_ADDR, BMI160_PMU_STATUS_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != BMI160_PMU_STATUS_VALUE_NORMALPOWERMODE) return QL_STATUS_ERROR;

	return QL_STATUS_OK;;
}

__PLATFORM_INIT__ QL_Status Accel_init(void)
{
#if 0
	//S3B_SENSOR_INTR_0 -> GPIO3
	//Configure GPIO in interrupt mode.

	GPIOCfgTypeDef  xGpioCfg;

	xGpioCfg.usPadNum = PAD_14;
	xGpioCfg.ucGpioNum = GPIO_3;
	xGpioCfg.ucFunc = PAD14_FUNC_SEL_GPIO_3;
	xGpioCfg.intr_type = LEVEL_TRIGGERED;//EDGE_TRIGGERED;
	xGpioCfg.pol_type = RISE_HIGH;//FALL_LOW;//RISE_HIGH;
	xGpioCfg.ucPull = PAD_NOPULL;

	HAL_GPIO_IntrCfg(&xGpioCfg);
#endif

	unsigned int index;

	accel_drv_priv.drvData.id = QL_SAL_SENSOR_ID_ACCEL;
	accel_drv_priv.drvData.type = QL_SDF_SENSOR_TYPE_BASE;		/* physical sensor */
	//accel_drv_priv.drvData.max_sample_rate = 0; 				// KK: this is not needed here at all, can be removed ?
	//accel_drv_priv.drvData.name = "Accel"; 					// KK: this is not used anywhere, can be removed ?

	accel_drv_priv.drvData.ops.open = M4_BMI160_Accel_open;
	accel_drv_priv.drvData.ops.close = M4_BMI160_Accel_close;
	accel_drv_priv.drvData.ops.read = M4_BMI160_Accel_read;
	accel_drv_priv.drvData.ops.probe = M4_BMI160_Accel_probe;
	accel_drv_priv.drvData.ops.ioctl = M4_BMI160_Accel_ioctl;
	accel_drv_priv.drvData.ops.suspend = M4_BMI160_Accel_suspend;
	accel_drv_priv.drvData.ops.resume = M4_BMI160_Accel_resume;

	/* the FFE side ID is common for all instances, it is a sensor/driver level detail */
	//accel_drv_priv.ffe_accel_id = QL_FFE_SENSOR_ID_ACCEL1;

	/* init framework level probe status to default, failed state - updated when probe is called */
	accel_drv_priv.drvData.probe_status = SENSOR_PROBE_STATUS_FAILED;

	/* all instances will default to the INVALID state, until probed */
	for ( index = 0 ; index < CONFIG_MAX_FFE_ACCEL_INSTANCE; index++ )
	{
		accel_drv_priv.devInstances[index].state = SENSOR_STATE_INVALID;
	}

	/* Do any platform related pin mux / I2C setting */

	/* register with SDF */
	if (QL_STATUS_OK != QL_SDF_SensorDrvRegister((struct QL_SDF_SensorDrv *) &accel_drv_priv))
	{
		QL_TRACE_SENSOR_ERROR("Failed to register Driver to SDF\n");
		return QL_STATUS_ERROR;
	}

//	AccelBtimer = xTimerCreate("Accel_BMI160_Btimer",pdMS_TO_TICKS(1000),pdTRUE, (void*)0xDEA,(TimerCallbackFunction_t)vAccelBtimerCB);
//
//	if(AccelBtimer == NULL){
//		QL_TRACE_SENSOR_ERROR(" %s: Failed to Create Batch Timer\n",__func__);
//		return QL_STATUS_ERROR;
//	}

	return QL_STATUS_OK;
}


#endif // PURE_M4_DRIVERS
