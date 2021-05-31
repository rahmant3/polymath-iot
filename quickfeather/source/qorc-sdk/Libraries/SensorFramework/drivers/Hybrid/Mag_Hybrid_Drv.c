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
 *    File   : Mag_Hybrid_Drv.c
 *    Purpose: 
 *                                                          
 *=========================================================*/
#include "Fw_global_config.h"

#include <stdio.h>
#include "QL_Trace.h"
#include "QL_SDF.h"
#include "QL_SensorIoctl.h"
#include "ffe_ipc_lib.h"
#include "QL_FFE_SensorConfig.h"
////#include "ql_testTask.h"
#include "Fw_global_config.h"
#include "QL_SensorCommon.h"                                  	// to remove warnings
#include "QL_SDF_Mag.h"

//#define __PLATFORM_INIT__

#ifdef HYBRID_FFE_M4_DRIVERS

#define SENSOR_STATE_INVALID	0
#define SENSOR_STATE_PROBED		1
#define SENSOR_STATE_OPENED		2

#define CONFIG_MAX_FFE_MAG_INSTANCE 2


#include <eoss3_hal_i2c.h>
QL_Status AK9915_Mag_Probe_In_M4();



/*
 * State Machine for the Sensor and its Internal Instances :
 *
 * all instances are at SENSOR_STATE_INVALID initially
 * if probe() is ok, all instances are moved to SENSOR_STATE_PROBED
 * if probe() fails, all instances are moved back to SENSOR_STATE_INVALID
 * open() will move state from SENSOR_STATE_PROBED -> SENSOR_STATE_OPENED
 * close() will move state from SENSOR_STATE_OPENED -> SENSOR_STATE_PROBED
 * we do not need SENSOR_STATE_CLOSED !
 * right now we do not have an explicit call to deinit the sensor, i.e. a complementary to probe() -> teardown() ?
 * (so we can transition from SENSOR_STATE_PROBED -> SENSOR_STATE_INVALID)
 * and if required, we will add this. currently we do not see any need for this.
 *
 */

/*
 * Private Structure to maintain the states of the sensor, as well as the states of individual instances held by clients *
 * FFE_SENSOR_ID will need to be maintained - common for all instances
 * FFE_INSTANCE_STATE - individually for each instance
 */

/* represents an instance of the sensor */
typedef struct
{
	unsigned char state;
	/* do we need any other variables for each instance ? */
}
QL_Mag_Ffe_Dev;

/*
 *
 * This is a private structure for the proxy driver running on M4.
 * The real driver is on FFE Side.
 * First member has to be QL_SDF_SensorDrv type as it is known by that type on the framework side.
 *
 */
typedef struct
{
	struct QL_SDF_SensorDrv drvData; 									/* Framework Side Representation */
	/* private stuff below */
	unsigned int  ffe_mag_id;											/* FFE Side Sensor ID */
	QL_Mag_Ffe_Dev devInstances[CONFIG_MAX_FFE_MAG_INSTANCE];		/* instance specific data */
}
QL_Mag_Ffe_Drv;


static QL_Mag_Ffe_Drv mag_drv_priv;

// private function declarations
QL_Status Mag_probe(struct QL_SDF_SensorDrv *s);
QL_Status Mag_Open(struct QL_SDF_SensorDrv *s, QL_SDF_SensorDrvInstanceHandle_t *instance_handle);
QL_Status Mag_Close(QL_SDF_SensorDrvInstanceHandle_t instance_handle);
QL_Status Mag_read(QL_SDF_SensorDrvInstanceHandle_t instance_handle, void *buf, int size);
QL_Status Mag_ioctl(QL_SDF_SensorDrvInstanceHandle_t instance_handle, unsigned int cmd, void *arg);
QL_Status Mag_suspend(QL_SDF_SensorDrvInstanceHandle_t instance_handle);
QL_Status Mag_resume(QL_SDF_SensorDrvInstanceHandle_t instance_handle);
//QL_Status Mag_FFE_IsArgValid(unsigned int cmd, void* arg);



static QL_Status Mag_probe(struct QL_SDF_SensorDrv *s)
{
	unsigned int index;

	QL_ASSERT(s != NULL);
	QL_ASSERT(s == (struct QL_SDF_SensorDrv *)&mag_drv_priv); // we should get back the same instance that we provided during registration

	/*
	 * probe should be called once, to check if sensor is alive, and the states of each instance should be changed
	 * to reflect the result of the probe.
	 *
	 */

	if(QL_STATUS_OK != AK9915_Mag_Probe_In_M4())
	{
		QL_TRACE_SENSOR_ERROR("AK9915_Mag_Probe_In_M4() failed for SAL sensor with ID = %d\n",mag_drv_priv.drvData.id);

		/* probe failed, update the state of each of the instances to not ready, SENSOR_STATE_INVALID */
		for ( index = 0 ; index < CONFIG_MAX_FFE_MAG_INSTANCE; index++ )
		{
			mag_drv_priv.devInstances[index].state = SENSOR_STATE_INVALID;
		}

		return QL_STATUS_ERROR;
	}
	QL_TRACE_SENSOR_DEBUG("AK9915_Mag_Probe_In_M4() OK for SAL sensor with ID = %d\n",mag_drv_priv.drvData.id);



	/* probe is ok, update the state of each of the instances to ready to use, SENSOR_STATE_PROBED */
	for ( index = 0 ; index < CONFIG_MAX_FFE_MAG_INSTANCE; index++ )
	{
		mag_drv_priv.devInstances[index].state = SENSOR_STATE_PROBED;
	}


	return QL_STATUS_OK;
}


static QL_Status Mag_Open(struct QL_SDF_SensorDrv *s, QL_SDF_SensorDrvInstanceHandle_t *instance_handle)
{
	unsigned int index ;

	*instance_handle = (QL_SDF_SensorDrvInstanceHandle_t)0xFFFF;
	QL_ASSERT(s != NULL);
	QL_ASSERT(s == (struct QL_SDF_SensorDrv *) &mag_drv_priv); // we should get back the same  that we provided during registration

	for ( index = 0 ; index < CONFIG_MAX_FFE_MAG_INSTANCE; index++ )
	{
//		printf("instance index: %d, state: %d\n",index, mag_drv_priv.devInstances[index].state);
		/* if we have an instance slot with state == SENSOR_STATE_PROBED, this is ready to use */
		if (SENSOR_STATE_PROBED == mag_drv_priv.devInstances[index].state)
		{
			/* move this instance's state to SENSOR_STATE_OPENED to mark that this slot is now in use */
			mag_drv_priv.devInstances[index].state = SENSOR_STATE_OPENED;

			/* pass back the index of the instance in our private table to Framework, as our "Instance Handle" */
			*instance_handle = (QL_SDF_SensorDrvInstanceHandle_t) index;

			/* indicate to framework that open succeeded */
			return QL_STATUS_OK;
		}
	}

	/* if we reach here, we do not have any free slots for further instances, indicate to framework that open failed */
	return QL_STATUS_ERROR;
}


static QL_Status Mag_Close(QL_SDF_SensorDrvInstanceHandle_t instance_handle)
{
	/* KK: check validity of handle. This is probably not needed and can be removed */
	QL_ASSERT((unsigned int) instance_handle < CONFIG_MAX_FFE_MAG_INSTANCE);

	/* move the state of this instance back to ready to use, SENSOR_STATE_PROBED */
	mag_drv_priv.devInstances[(unsigned int)instance_handle].state = SENSOR_STATE_PROBED;

	/* indicate to framework that the instance was closed successfully */
	return QL_STATUS_OK;
}


static QL_Status Mag_read(QL_SDF_SensorDrvInstanceHandle_t instance_handle, void *buf, int size)
{
	/* KK: read data from sensor is not implemented yet, no use case ? */
	return QL_STATUS_NOT_IMPLEMENTED;
}

#if 0 //Not used
static QL_Status Mag_FFE_IsArgValid(unsigned int cmd, void* arg)
{
	/* for all the "set"/"get" calls, we check that the arguments passed in are valid */
	/* for example, Magerometer Dynamic Range -> arg = 0/1/2/3, any other is invalid */
	/* this function has to be tailored for the specific sensor instantiated, it will differ, if a different sensor is employed */
	switch (cmd)
	{
		case QL_SAL_IOCTL_SET_DYNAMIC_RANGE:
		{
			unsigned int dynamicRange = *(unsigned int *)arg;
			if(!(dynamicRange <= 3)) return QL_STATUS_ERROR;
		}
		break;

		case QL_SAL_IOCTL_SET_ODR:
		{
			unsigned int odr = *(unsigned int *)arg;
			if(!((odr >= 25) && (odr <= 1600) && (odr%25 == 0)))  return QL_STATUS_ERROR;
		}
		break;
	}

	/* default, for unknown command, we do not know to check for validity, so it will be a dummy OK */
	/* alternative is to return error by default, so that we have a check for each and every command possible added here ? */
	return QL_STATUS_OK;
}
#endif


static QL_Status Mag_ioctl(QL_SDF_SensorDrvInstanceHandle_t instance_handle, unsigned int cmd, void *arg)
{
	unsigned int sensorid;
	QL_Status status = QL_STATUS_OK;

	sensorid = mag_drv_priv.ffe_mag_id;
	QL_ASSERT( (unsigned int)instance_handle < CONFIG_MAX_FFE_MAG_INSTANCE);

//	printf("sensorid: %u, cmd: %u\r\n", sensorid, cmd);

	/* check if the instance is in open state first */
	if (SENSOR_STATE_OPENED != mag_drv_priv.devInstances[(unsigned int) instance_handle].state)
	{
		return QL_STATUS_ERROR;
	}

	/* before invoking the command implementation, ensure that the arguments are valid */
	/* the arg check function *must* be specific to the end sensor being used and has to be checked ! */
//	if(QL_STATUS_OK != Mag_FFE_IsArgValid(cmd, arg))
//	{
//		QL_TRACE_SENSOR_ERROR("Command %d Arguments Invalid for sensor ID = %d\n", cmd, sensorid);
//		return QL_STATUS_ERROR;
//	}

	switch (cmd)
	{

		case QL_SAL_IOCTL_SET_CALLBACK:
		{
			mag_drv_priv.drvData.sensor_callback = (QL_SensorEventCallback_t)arg;
		}
		break;

		case QL_SAL_IOCTL_GET_CALLBACK:
		{
			*(QL_SensorEventCallback_t *)arg = mag_drv_priv.drvData.sensor_callback;
		}
		break;

		case QL_SAL_IOCTL_BATCH_ENABLE:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_BATCH_ENABLE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_BATCH_ENABLE failed for FFE sensor ID = %d \n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_BATCH_GET_ENABLE:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_BATCH_GET_ENABLE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_BATCH_GET_ENABLE failed for FFE sensor ID = %d \n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;
		case QL_SAL_IOCTL_SET_BATCH:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_SET_BATCH, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_SET_BATCH failed for FFE sensor ID = %d \n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_GET_BATCH:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_BATCH, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_GET_BATCH failed for FFE sensor ID = %d \n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;


		case QL_SAL_IOCTL_BATCH_FLUSH:
		{
//			static int flushcounter=0;
//			printf ( "batch flush count = %d\n", ++flushcounter); //TBD: take it out
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_BATCH_FLUSH, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_BATCH_FLUSH failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_ENABLE:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_ENABLE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_ENABLE failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;


		case QL_SAL_IOCTL_GET_ENABLE:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_ENABLE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_GET_ENABLE failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_ALLOC_DATA_BUF:
		{
			// call the platform function

			if (QL_STATUS_OK != QL_SDF_SensorAllocMem(	sensorid,
					(void **)&(((struct QL_SF_Ioctl_Set_Data_Buf*)arg)->buf) ,
					((struct QL_SF_Ioctl_Set_Data_Buf*)arg)->size) )
			{
				QL_TRACE_SENSOR_ERROR("QL_SDF_SensorAllocMem failed to get memory for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_SET_DATA_BUF:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_SET_MEM_BUFFER, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_SET_MEM_BUFFER failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_GET_DATA_BUF:
		{
			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_MEM_BUFFER, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_GET_MEM_BUFFER failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_LIVE_DATA_ENABLE:
			{
				if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_LIVE_DATA_ENABLE, arg))
				{
					QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_LIVE_DATA_ENABLE failed for FFE sensor ID = %d\n", sensorid);
					status = QL_STATUS_ERROR;
				}
			}
			break;

			case QL_SAL_IOCTL_GET_LIVE_DATA_ENABLE:
			{
				if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_LIVE_DATA_ENABLE, arg))
				{
					QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_LIVE_DATA_ENABLE failed for FFE sensor ID = %d\n", sensorid);
					status = QL_STATUS_ERROR;
				}
			}
			break;

		case QL_SAL_IOCTL_SET_DYNAMIC_RANGE:
		{

			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_SET_DYNAMIC_RANGE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_SET_DYNAMIC_RANGE failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_GET_DYNAMIC_RANGE:
		{

			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_DYNAMIC_RANGE, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_GET_DYNAMIC_RANGE failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_SET_ODR:
		{

			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_SET_ODR, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_SET_ODR failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		case QL_SAL_IOCTL_GET_ODR:
		{

			if (QL_STATUS_OK != QL_FFE_OperationForSensor(sensorid, QL_FFE_SENSOR_OPS_GET_ODR, arg))
			{
				QL_TRACE_SENSOR_ERROR("QL_FFE_SENSOR_OPS_GET_ODR failed for FFE sensor ID = %d\n", sensorid);
				status = QL_STATUS_ERROR;
			}
		}
		break;

		default:
		{
			/* ignore unknown IOCTLs */
			status = QL_STATUS_ERROR;
		}
		break;
	}

	return status;
}


static QL_Status Mag_suspend(QL_SDF_SensorDrvInstanceHandle_t instance_handle)
{
	/* no use case for suspend of sensor instance */
	return QL_STATUS_NOT_IMPLEMENTED;
}


static QL_Status Mag_resume(QL_SDF_SensorDrvInstanceHandle_t instance_handle)
{
	/* no use case for resume of sensor instance */
	return QL_STATUS_NOT_IMPLEMENTED;
}


__PLATFORM_INIT__ QL_Status Mag_init()
{
	unsigned int index;

	mag_drv_priv.drvData.id = QL_SAL_SENSOR_ID_MAG;
	mag_drv_priv.drvData.type = QL_SDF_SENSOR_TYPE_BASE;		/* physical sensor */
	//mag_drv_priv.drvData.max_sample_rate = 0; 				// KK: this is not needed here at all, can be removed ?
	//mag_drv_priv.drvData.name = "Mag"; 					// KK: this is not used anywhere, can be removed ?

	mag_drv_priv.drvData.ops.open = Mag_Open;
	mag_drv_priv.drvData.ops.close = Mag_Close;
	mag_drv_priv.drvData.ops.read = Mag_read;
	mag_drv_priv.drvData.ops.probe = Mag_probe;
	mag_drv_priv.drvData.ops.ioctl = Mag_ioctl;
	mag_drv_priv.drvData.ops.suspend = Mag_suspend;
	mag_drv_priv.drvData.ops.resume = Mag_resume;

	/* the FFE side ID is common for all instances, it is a sensor/driver level detail */
	mag_drv_priv.ffe_mag_id = QL_FFE_SENSOR_ID_MAG;

	/* init framework level probe status to default, failed state - updated when probe is called */
	mag_drv_priv.drvData.probe_status = SENSOR_PROBE_STATUS_FAILED;

	/* all instances will default to the INVALID state, until probed */
	for ( index = 0 ; index < CONFIG_MAX_FFE_MAG_INSTANCE; index++ )
	{
		mag_drv_priv.devInstances[index].state = SENSOR_STATE_INVALID;
	}

	/* Do any platform related pin mux / I2C setting */

	/* register with SDF */
	if (QL_STATUS_OK != QL_SDF_SensorDrvRegister((struct QL_SDF_SensorDrv *) &mag_drv_priv))
	{
		QL_TRACE_SENSOR_ERROR("Failed to register Driver to SDF\n");
		return QL_STATUS_ERROR;
	}

	return QL_STATUS_OK;
}



#define AK9915_DEV_I2C_ADDR         	0x0D
#define AK9915_DEV_I2C_BUS				0x00

#define AK9915_WIA1_REG      			0x00 //Chip ID Register (2 Bytes 0x00 = Company ID, 0x01 = Device ID)
#define AK9915_WIA2_REG      			0x01
#define AK9915_CNTL2_REG   				0x31 //for Mode Setup
#define AK9915_CNTL3_REG  				0x32 //for Soft Reset

#define AK9915_SOFT_RESET_VALUE			0x01 // to initiate softreset
#define AK9915_WIA1_RESET_VALUE 		0x48
#define AK9915_WIA2_RESET_VALUE			0x10
#define AK9915_CNTL2_RESET_VALUE		0x00
#define AK9915_CNTL3_RESET_VALUE		0x00

QL_Status AK9915_Mag_Probe_In_M4()
{
	/*
	 * BMI 160 MAG probe sequence
	 *
	 * (1) Read CHIP ID and verify (CHIP ID REG)
	 * (2) Soft Reset ???
	 * (3) Wait for 50 ms after Soft Reset, Check if any error (ERROR STATUS REG)
	 * (4) Set default Rate(ODR), range and power mode(LP/HP)
	 * (5) Wait atleast 5ms, Check if any error (ERROR STATUS REG) and power mode (PMU STATUS REG) is as set
	 *
	 */

	I2C_Config xI2CConfig;
	uint8_t i2c_read_data = 0x0;
	uint8_t i2c_write_data = 0x0;

	xI2CConfig.eI2CFreq = I2C_200KHZ;
	xI2CConfig.eI2CInt = I2C_DISABLE;
	xI2CConfig.ucI2Cn = AK9915_DEV_I2C_BUS;					/* SM0 == I2C0 */

	HAL_I2C_Init(xI2CConfig);

	/* chip id check */
	if(HAL_I2C_Read(AK9915_DEV_I2C_ADDR, AK9915_WIA1_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != AK9915_WIA1_RESET_VALUE) return QL_STATUS_ERROR;

	printf("AK9915 chip id check OK (AK9915_WIA1_RESET_VALUE = 0x%x)\n", i2c_read_data);

	if(HAL_I2C_Read(AK9915_DEV_I2C_ADDR, AK9915_WIA2_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != AK9915_WIA2_RESET_VALUE) return QL_STATUS_ERROR;

	printf("AK9915 chip id check OK (AK9915_WIA2_RESET_VALUE = 0x%x)\n", i2c_read_data);


	/* soft reset */
	i2c_write_data = AK9915_SOFT_RESET_VALUE;
	if( HAL_I2C_Write(AK9915_DEV_I2C_ADDR, AK9915_CNTL3_REG, &i2c_write_data, 1) != HAL_OK ) return QL_STATUS_ERROR;

	/* wait for 50ms */
	volatile int x = 50000;
	while(x--);

	/* check for any error */
	if(HAL_I2C_Read(AK9915_DEV_I2C_ADDR, AK9915_CNTL2_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != AK9915_CNTL2_RESET_VALUE) return QL_STATUS_ERROR;
	printf("AK9915_CNTL2_REG = 0x%x\n", i2c_read_data);

	if(HAL_I2C_Read(AK9915_DEV_I2C_ADDR, AK9915_CNTL3_REG, &i2c_read_data, 1) != HAL_OK) return QL_STATUS_ERROR;
	if(i2c_read_data != AK9915_CNTL3_RESET_VALUE) return QL_STATUS_ERROR;
	printf("AK9915_CNTL3_REG = 0x%x\n", i2c_read_data);

	printf("AK9915 Soft Reset OK, no error after reset\n");

	/* at this point probe() is ok */
	printf("AK9915 Probe on M4 OK\n\n");

	return QL_STATUS_OK;
}

#endif // HYBRID_FFE_M4_DRIVERS
