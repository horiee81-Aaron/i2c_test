#include "r_config_iica0.h"
#include "mock_r_config_iica0.h"

static mock_r_config_iica0_state_t g_state = {0};

void MOCK_R_Config_IICA0_Reset(void)
{
    g_state.stop_calls = 0U;
    g_state.create_calls = 0U;
    g_state.start_calls = 0U;
    g_state.slave_receive_start_calls = 0U;
}

const mock_r_config_iica0_state_t *MOCK_R_Config_IICA0_GetState(void)
{
    return &g_state;
}

void R_Config_IICA0_Stop(void)
{
    g_state.stop_calls++;
}

void R_Config_IICA0_Create(void)
{
    g_state.create_calls++;
}

void R_Config_IICA0_Start(void)
{
    g_state.start_calls++;
}

void R_Config_IICA0_SlaveReceiveStart(void)
{
    g_state.slave_receive_start_calls++;
}