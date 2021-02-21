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
#include "pm_training.h"

// --------------------------------------------------------------------------------------------------------------------
// STRUCTURE AND STRUCTURE TYPEDEFS
// --------------------------------------------------------------------------------------------------------------------
typedef struct PmTrainingConfig_s
{
    uint8_t numdims; //!< Number of 16-bit data points in the data.
} PmTrainingConfig_t;

// --------------------------------------------------------------------------------------------------------------------
// VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static PmTrainingModule_t g_trainingModule = PmTrainingNone; //!< The current module being trained.
static bool g_trainingEnabled = false; //!< Is the current module being trained?

static PmTrainingConfig_t configs[PmNumTrainingModules] =
{
    [PmTrainingNone]       = { .numdims = 0 },
    [PmTrainingUltrasound] = { .numdims = 1 },
    [PmTrainingAir]        = { .numdims = 4 },
    [PmTrainingAccel]      = { .numdims = 3 }
};

// --------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
void pm_training_ultrasound(const struct cli_cmd_entry *pEntry)
{
    UNUSED(pEntry);
    g_trainingModule = PmTrainingUltrasound;
}

void pm_training_air(const struct cli_cmd_entry *pEntry)
{
    UNUSED(pEntry);
    g_trainingModule = PmTrainingAir;
}

void pm_training_set(const struct cli_cmd_entry *pEntry)
{
    PmTrainingModule_t module = (PmTrainingModule_t)pEntry->cookie;
    
    if (module < PmNumTrainingModules)
    {
        g_trainingModule = (PmTrainingModule_t)pEntry->cookie;
    }
}

void pm_training_start(const struct cli_cmd_entry *pEntry)
{
    UNUSED(pEntry);
    g_trainingEnabled = true;
}

void pm_training_stop(const struct cli_cmd_entry *pEntry)
{
    UNUSED(pEntry);
    g_trainingEnabled = false;
}

void pm_training_output(const struct cli_cmd_entry *pEntry)
{
    UNUSED(pEntry);

    if (g_trainingEnabled)
    {
        CLI_printf("Training is still running. Stop the current training set.\n\n");
    }
    else
    {
        // TODO
    }

}

void pm_training_state(bool * enabled, PmTrainingModule_t * module)
{
    if ((NULL != enabled) && (NULL != module))
    {
        *enabled = g_trainingEnabled;
        *module = g_trainingModule;
    }
}
