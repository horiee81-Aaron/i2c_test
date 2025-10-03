#include "hal_i2c_slave.h"
#include "r_config_iica0.h"

void R_Config_IICA0_ResetBusLines(void)
{
    /* Host-side build does not toggle physical bus lines. */
}

void R_Config_IICA0_SlaveStartCallback(uint8_t status_flags)
{
    HAL_I2C_S_OnStartCondition(status_flags);
}

void R_Config_IICA0_SlaveReceiveCallback(uint8_t data_byte)
{
    HAL_I2C_S_OnByteReceived(data_byte);
}

void R_Config_IICA0_SlaveStopCallback(uint8_t status_flags)
{
    HAL_I2C_S_OnStopCondition(status_flags);
}

void R_Config_IICA0_ErrorCallback(uint8_t status_flags)
{
    HAL_I2C_S_OnHardwareError(status_flags);
}
