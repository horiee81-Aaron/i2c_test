#include "hal_scheduler.h"
#include "mock_hal_scheduler.h"

static uint32_t g_uptime_ms = 0U;

void MOCK_HAL_SCHED_Reset(void)
{
    g_uptime_ms = 0U;
}

void MOCK_HAL_SCHED_SetUptime(uint32_t value)
{
    g_uptime_ms = value;
}

void MOCK_HAL_SCHED_Advance(uint32_t delta_ms)
{
    g_uptime_ms += delta_ms;
}

uint32_t HAL_SCHED_GetUptimeMs(void)
{
    return g_uptime_ms;
}