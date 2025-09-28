#include "hal_i2c.h"
#include "r_config_iica0.h"

void R_Config_IICA0_SlaveStartCallback(uint8_t status_flags)
{
    HAL_I2C_OnStartCondition(status_flags);
}

void R_Config_IICA0_SlaveReceiveCallback(uint8_t data_byte)
{
    HAL_I2C_OnByteReceived(data_byte);
}

void R_Config_IICA0_SlaveStopCallback(uint8_t status_flags)
{
    HAL_I2C_OnStopCondition(status_flags);
}

void R_Config_IICA0_ErrorCallback(uint8_t status_flags)
{
    HAL_I2C_OnHardwareError(status_flags);
}
