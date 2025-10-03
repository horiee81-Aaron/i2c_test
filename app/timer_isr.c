#include "hal_i2c_slave.h"
#include "hal_scheduler.h"
#include "r_cg_macrodriver.h"

#pragma interrupt INTTM00 TM00_ISR
void TM00_ISR(void)
{
    HAL_SCHED_TickISR();
    HAL_I2C_S_Tick1ms();
}
