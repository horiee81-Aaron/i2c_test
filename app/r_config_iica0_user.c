#include "hal_i2c_slave.h"
#include "r_config_iica0.h"
#include "r_cg_macrodriver.h"

static void r_iica0_delay_cycles(uint16_t cycles)
{
    volatile uint16_t counter = cycles;

    while (counter-- != 0U)
    {
        /* Busy-wait to let the bus settle */
    }
}

void R_Config_IICA0_ResetBusLines(void)
{
    R_Config_IICA0_Stop();

#if defined(P6) && defined(PM6)
    const uint8_t scl_mask  = (uint8_t)(1U << 1);
    const uint8_t sda_mask  = (uint8_t)(1U << 0);
    const uint8_t pins_mask = (uint8_t)(scl_mask | sda_mask);
    const uint8_t p6_backup  = P6;
    const uint8_t pm6_backup = PM6;
#if defined(PMC6)
    const uint8_t pmc6_backup = PMC6;
#endif
#if defined(PU6)
    const uint8_t pu6_backup = PU6;
#endif

#if defined(PMC6)
    PMC6 &= (uint8_t)(~pins_mask);
#endif
#if defined(PU6)
    PU6 |= pins_mask;
#endif

    PM6 |= pins_mask;

    for (uint8_t pulse = 0U; pulse < 9U; ++pulse)
    {
        PM6 &= (uint8_t)(~scl_mask);
        P6 &= (uint8_t)(~scl_mask);
        r_iica0_delay_cycles(UINT16_C(200));

        PM6 |= scl_mask;
        r_iica0_delay_cycles(UINT16_C(200));

        if ((P6 & sda_mask) != 0U)
        {
            break;
        }
    }

    PM6 &= (uint8_t)(~sda_mask);
    P6 &= (uint8_t)(~sda_mask);
    r_iica0_delay_cycles(UINT16_C(200));

    PM6 &= (uint8_t)(~scl_mask);
    P6 &= (uint8_t)(~scl_mask);
    r_iica0_delay_cycles(UINT16_C(200));

    PM6 |= scl_mask;
    r_iica0_delay_cycles(UINT16_C(200));

    PM6 |= sda_mask;
    r_iica0_delay_cycles(UINT16_C(200));

#if defined(PU6)
    PU6 = (uint8_t)((PU6 & (uint8_t)(~pins_mask)) | (pu6_backup & pins_mask));
#endif
    P6  = (uint8_t)((P6 & (uint8_t)(~pins_mask)) | (p6_backup & pins_mask));
    PM6 = (uint8_t)((PM6 & (uint8_t)(~pins_mask)) | (pm6_backup & pins_mask));
#if defined(PMC6)
    PMC6 = (uint8_t)((PMC6 & (uint8_t)(~pins_mask)) | (pmc6_backup & pins_mask));
#endif
#endif /* defined(P6) && defined(PM6) */
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
