#ifndef PM_PROTOCOL_DEFS_H
#define PM_PROTOCOL_DEFS_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the core definitions needed to implement the master-slave protocol for polymath-iot.
//   
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdint.h>

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------

//! Protocol identifier (32-bit number). This should never change for this protocol.
#define PM_PROTOCOL_IDENTIFIER 0x05050505

//! The maximum number of sensors assumed to be attached to the slave.
#define PM_MAX_SENSORS_PER_CLUSTER (8u)

// --------------------------------------------------------------------------------------------------------------------
// ENUMS AND ENUM TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef enum pmSensorIds_e
{
  PM_SENSOR_BME680_PRESSURE = 0xE0,
  PM_SENSOR_BME680_TEMPERATURE = 0xE1,
  PM_SENSOR_BME680_HUMIDITY = 0xE2,
  
  PM_SENSOR_BME680_GAS_CO2  = 0xE3,
  PM_SENSOR_BME680_GAS_RAW  = 0xE4,
  
  PM_SENSOR_MICS5524_GAS_CO = 0xE5,
  PM_SENSOR_MICS5524_GAS_CH4 = 0xE6,
  PM_SENSOR_MICS5524_GAS_RAW = 0xE7,
  
  PM_SENSOR_B5W_DUST_PPM = 0xE8,
  PM_SENSOR_HCSR04_DIST = 0xE9,
  
} pmSensorIds_t;

typedef enum pmCommandCodes_e
{
  //! No parameters are included in this request. Used by the master to determine if the slave is compatible with
  //! the master. The slave shall return with a structure of type pmCmdProtocolInfo_t.
  PM_CMD_PROTOCOL_ID = 0x00,

  //! No parameters are included in this request. Used by the master to read the cluster ID connected to the slave. The
  //! slave shall return with a structure of type pmCmdClusterId_t.
  PM_CMD_CLUSTER_ID  = 0x01,

  //! No parameters are included in this request. Used by the master to obtain the identifiers data for each sensor on 
  //! the cluster. The slave shall return with a structure of type pmCmdGetSensors_t, containing up to 
  //! PM_MAX_SENSORS_PER_CLUSTER identifiers and data.
  PM_CMD_GET_SENSORS = 0x02,

  //! A single byte is included in this request. If 0x00, indicates Success status -- this is a signal to the slave
  //! controller that we can begin normal operation. if 0x01, indicates a Error status -- unrecoverable error occurred.
  //! The slave shall return with a structure of type pmCmdWriteStatus_t, echoing the data that was written.
  PM_CMD_WRITE_STATUS = 0x03,

  // 0x04-0xEF reserved.

  // Note that a positive ACK is the corresponding command byte (above) OR'd with 0x80.

  //! These are always returned by the slave in response to the above commands.
  PM_CMD_NEGATIVE_ACK = 0xFF
} pmCommandCodes_t;

typedef enum pmStatusCodes_e
{
  //! Sent by the master to indicate slave can begin operation.
  PM_RET_SUCCESS = 0x00,

  //! Sent by the master to indicate an error has occurred and slave should stop operation.
  PM_RET_FAILURE = 0x01
} pmStatusCodes_t;

// --------------------------------------------------------------------------------------------------------------------
// STRUCTURES AND STRUCTURE TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef struct pmSensorReading_s
{
  uint16_t sensorId;
  uint16_t data;
} pmSensorReading_t;

//! No params is included in this request. Is is assumed each uint32_t is LSB.
typedef struct pmCmdProtocolInfo_s
{
  uint32_t protocolIdentifier;
} pmCmdProtocolInfo_t;

//! No params is included in this request. Is is assumed each uint32_t is LSB.
typedef struct pmCmdClusterId_s
{
  uint32_t clusterIdentifier;
} pmCmdClusterId_t;

//! No params is included in this request.
typedef struct pmCmdGetSensors_s
{
  pmSensorReading_t sensors[PM_MAX_SENSORS_PER_CLUSTER];
  uint16_t numSensors;
} pmCmdGetSensors_t;

//! A single byte is included in this request, containing the status from the master.
typedef struct pmCmdWriteStatus_s
{
  uint8_t status; //!< One of the status codes in #pmStatusCodes_t.
} pmCmdWriteStatus_t;

//! Response structure sent by the slave in response to a command, received by the master.
typedef struct pmCmdPacketDefinition_s
{
  uint8_t commandCode; //!< One of the command codes in #pmCommandCodes_t.
  union
  {
    pmCmdProtocolInfo_t protocolInfo;
    pmCmdClusterId_t clusterId;
    pmCmdGetSensors_t getSensors;
    pmCmdWriteStatus_t writeStatus;
  };
} pmCmdPayloadDefinition_t;


#endif // PM_PROTOCOL_DEFS_H