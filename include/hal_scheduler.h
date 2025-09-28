#ifndef HAL_SCHEDULER_H
#define HAL_SCHEDULER_H

#include <stdint.h>

typedef void (*hal_sched_task_fn_t)(void);

typedef struct
{
    hal_sched_task_fn_t function;
    uint32_t            next_deadline;
    uint16_t            period_ticks;
} hal_sched_task_t;

void HAL_SCHED_Init(uint16_t tick_hz);
void HAL_SCHED_RegisterTasks(hal_sched_task_t *tasks, uint8_t task_count);
void HAL_SCHED_TickISR(void);
void HAL_SCHED_RunOnce(void);
uint32_t HAL_SCHED_GetUptimeMs(void);

#endif /* HAL_SCHEDULER_H */
