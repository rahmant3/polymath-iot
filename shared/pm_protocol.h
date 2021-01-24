#ifndef PM_PROTOCOL_H
#define PM_PROTOCOL_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the communication protocol between the master and slave modules on the polymath-iot module.
//
//   All communication between the master and slave module follow this structure:
//   - The master sends a message of type pmRequest_t, where the first byte contains the command code (see 
//     pmCommandCodes_t), and the next 7 bytes contain parameters. To simplify the protocol, the command will 
//     always contain 7 bytes of parameters, even if it is not used.
//
//   - The slave responds with a message of type pmResponse_t, where the first byte contains the response code (see
//     pmReturnCodes_t), and the next bytes are variable length, depending on the command code that was sent by the
//     master.
//
// TODO: Once this basic protocol is working, it should be improved to be more data efficient (i.e. not requiring one
//       SPI transaction to obtain 1 sample from a sensor).
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdint.h>

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------

// Protocol identifier (32-bit number). This should never change for this protocol.
#define PM_PROTOCOL_IDENTIFIER 0x05050505

// Compatibility number. Increment this by one each time the protocol changes and the master and slave are not 
// compatible.
#define PM_PROTOCOL_COMPATIBILITY_VERSION (1u) 

// The maximum number of sensors assumed to be attached to the slave.
#define PM_MAX_SENSORS_PER_CLUSTER (8u)

// --------------------------------------------------------------------------------------------------------------------
// ENUMS AND ENUM TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef enum pmCommandCodes_e
{
  // No parameters are included in this request. Used by the master to determine if the slave is compatible with
  // the master. The slave shall return with a structure of type pmCmdProtocolInfo_t.
  PM_CMD_PROTOCOL_INFO,

  // No parameters are included in this request. Used by the master to determine how many sensors are on the
  // sensor cluster. The slave shall return with a structure of type pmCmdNumSensors_t, containing the number of
  // sensors attached (up to PM_MAX_SENSORS_PER_CLUSTER).
  PM_CMD_NUM_SENSORS,

  // No parameters are included in this request. Used by the master to determine the identifiers for each sensor on 
  // the cluster. The slave shall return with a structure of type pmCmdGetSensors_t, containing up to 
  // PM_MAX_SENSORS_PER_CLUSTER identifiers.
  PM_CMD_GET_SENSORS,

  // Byte 0-3 contains the ID of the sensor to read (LSB). Used by the master to get the sample rate of a sensor. The
  // slave shall return with a structure of type pmCmdGetSampleRate_t, containing a 16-bit sample rate in Hz (LSB).
  PM_CMD_GET_SAMPLE_RATE,

  // Byte 0-3 contains the ID of the sensor to read (LSB). Used by the master to obtain a reading from a sensor. The
  // slave shall return with a structure of type pmCmdGetSample_t, containing a 16-bit sample (LSB).
  PM_CMD_GET_SAMPLE
} pmCommandCodes_t;

typedef enum pmReturnCodes_e
{
  // Returned by the slave to indicate successful operation.
  PM_RET_SUCCESS,

  // Returned by the slave to indicate failed operation. No additional information available.
  PM_RET_FAILURE
} pmReturnCodes_t;


// --------------------------------------------------------------------------------------------------------------------
// STRUCTURES AND STRUCTURE TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------

// Command structure sent by the master, received by the slave.
typedef struct pmRequest_s
{
  uint8_t command;
  uint8_t params[3];
} pmRequest_t;

// No params is included in this request. Is is assumed each uint32_t is LSB.
typedef struct pmCmdProtocolInfo_s
{
  uint32_t protocolIdentifier;
  uint32_t protocolCompatibility;
} pmCmdProtocolInfo_t;

// No params is included in this request. Is is assumed each uint32_t is LSB.
typedef struct pmCmdNumSensors_s
{
  uint32_t numSensors;
} pmCmdNumSensors_t;

// No params is included in this request. Is is assumed each uint32_t is LSB.
typedef struct pmCmdGetSensors_s
{
  uint32_t sensors[PM_MAX_SENSORS_PER_CLUSTER];
} pmCmdGetSensors_t;

// Byte 0-3 contains the ID of the sensor to read (LSB).
typedef struct pmCmdGetSample_s
{
  uint16_t sample;
} pmCmdGetSample_t;

// Byte 0-3 contains the ID of the sensor to read (LSB).
typedef struct pmCmdGetSampleRate_s
{
  uint16_t sampleRate_Hz;
} pmCmdGetSampleRate_t;


// Response structure sent by the slave in response to a command, received by the master.
typedef struct pmResponse_s
{
  uint8_t returnCode;
  union
  {
    pmCmdProtocolInfo_t protocolInfo;
    pmCmdNumSensors_t numSensors;
    pmCmdGetSensors_t getSensors;
    pmCmdGetSample_t getSample;
    pmCmdGetSampleRate_t getRate;
  };
} pmResponse_t;

#endif // PM_PROTOCOL_H