#ifndef MOCK_HAL_SCHEDULER_H
#define MOCK_HAL_SCHEDULER_H

#include <stdint.h>

void MOCK_HAL_SCHED_Reset(void);
void MOCK_HAL_SCHED_SetUptime(uint32_t value);
void MOCK_HAL_SCHED_Advance(uint32_t delta_ms);

#endif /* MOCK_HAL_SCHEDULER_H */