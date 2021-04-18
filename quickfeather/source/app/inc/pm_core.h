#ifndef PM_CORE_H
#define PM_CORE_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   This file defines the interface to the polymath application.
//
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------

typedef enum pmCoreModes_e
{
	PM_MODE_STARTUP, //!< Currently starting up.

	PM_MODE_TEST_SLAVE, //!< In slave test mode.
	PM_MODE_TEST_BLE,   //!< In BLE test mode.

	PM_MODE_WAITING, //!< Waiting for a cluster to be connected.
	PM_MODE_PAIRING, //!< Pairing with a connected cluster.
	PM_MODE_NORMAL,  //!< Normal operating mode.

	PM_MODE_ERROR //!< In an error mode.
} pmCoreModes_t;

void pm_main(void);
void pm_test_send(const char *s);

void pmSetMode(pmCoreModes_t mode);
pmCoreModes_t pmGetMode(void);

#endif // PM_CORE_H
