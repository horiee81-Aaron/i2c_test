#include "hal_scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static hal_sched_task_t *g_task_table = NULL;
static uint8_t            g_task_count = 0U;
static volatile uint32_t  g_uptime_ticks = 0UL;
static uint16_t           g_tick_hz = 0U;

static bool hal_sched_is_time_due(uint32_t current, uint32_t deadline);

static bool hal_sched_is_time_due(uint32_t current, uint32_t deadline)
{
    const uint32_t delta = current - deadline;
    bool is_due = false;

    if (delta < UINT32_C(0x80000000))
    {
        is_due = true;
    }
    else
    {
        /* No action required */
    }

    return is_due;
}

void HAL_SCHED_Init(uint16_t tick_hz)
{
    g_tick_hz      = tick_hz;
    g_uptime_ticks = 0UL;
    g_task_table   = NULL;
    g_task_count   = 0U;
}

void HAL_SCHED_RegisterTasks(hal_sched_task_t *tasks, uint8_t task_count)
{
    if ((tasks == NULL) || (task_count == 0U))
    {
        g_task_table = NULL;
        g_task_count = 0U;
    }
    else
    {
        uint8_t index;
        const uint32_t now = g_uptime_ticks;

        g_task_table = tasks;
        g_task_count = task_count;

        for (index = 0U; index < g_task_count; index++)
        {
            hal_sched_task_t *task = &g_task_table[index];
            task->next_deadline = now + (uint32_t)task->period_ticks;
        }
    }
}

void HAL_SCHED_TickISR(void)
{
    g_uptime_ticks++;
}

void HAL_SCHED_RunOnce(void)
{
    uint8_t index;
    const uint32_t now = g_uptime_ticks;

    if (g_task_table == NULL)
    {
        return;
    }

    for (index = 0U; index < g_task_count; index++)
    {
        hal_sched_task_t *task = &g_task_table[index];

        if (task->function != NULL)
        {
            if (hal_sched_is_time_due(now, task->next_deadline) != false)
            {
                task->function();

                if (task->period_ticks == 0U)
                {
                    task->next_deadline = now + UINT32_C(1);
                }
                else
                {
                    do
                    {
                        task->next_deadline += (uint32_t)task->period_ticks;
                    }
                    while (hal_sched_is_time_due(now, task->next_deadline) != false);
                }
            }
            else
            {
                /* No action required */
            }
        }
        else
        {
            /* No action required */
        }
    }
}

uint32_t HAL_SCHED_GetUptimeMs(void)
{
    uint32_t uptime_ms = 0UL;

    if (g_tick_hz > 0U)
    {
        const uint32_t ticks_snapshot = g_uptime_ticks;
        const uint64_t scaled = ((uint64_t)ticks_snapshot * UINT64_C(1000));

        uptime_ms = (uint32_t)(scaled / (uint64_t)g_tick_hz);
    }
    else
    {
        /* No action required */
    }

    return uptime_ms;
}
