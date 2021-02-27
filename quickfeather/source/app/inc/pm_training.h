#ifndef PM_TRAINING_H
#define PM_TRAINING_H

// --------------------------------------------------------------------------------------------------------------------
// Repository:
// - Github: https://github.com/rahmant3/polymath-iot
//
// Description:
//   Provides the interface used to perform training on the polymath system.
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// INCLUDES
// --------------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>

#include <Fw_global_config.h>
#include <cli.h>

// --------------------------------------------------------------------------------------------------------------------
// ENUMS AND ENUM TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef enum PmTrainingModule_e
{
    PmTrainingNone,
    PmTrainingUltrasound,
    PmTrainingAir,
    PmTrainingAccel,

    PmNumTrainingModules
} PmTrainingModule_t;

// --------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------------------------------------------

/**
 * @brief Set the active training model to one of the options in #PmTrainingModule_t.
 * 
 * @param pEntry The member pEntry->cookie is expected to include one of the options in #PmTrainingModule_t.
 */
void pm_training_set(const struct cli_cmd_entry *pEntry);

/**
 * @brief Start recording training data.
 * 
 * @param pEntry Not used.
 */
void pm_training_start(const struct cli_cmd_entry *pEntry);

/**
 * @brief Stop recording training data.
 * 
 * @param pEntry Not used.
 */
void pm_training_stop(const struct cli_cmd_entry *pEntry);

/**
* @brief Call to obtain the training status.
* 
* @param enabled The training state is loaded to this variable (true on enabled, false otherwise).
* @param module The training module is loaded to this variable (see #PmTrainingModule_t).
*/
void pm_training_state(bool * enabled, PmTrainingModule_t * module);


#endif // PM_TRAINING_H
