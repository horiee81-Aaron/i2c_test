#include "app_i2c_registers.h"

static const uint8_t g_app_i2c_hw_version[] = APP_I2C_HW_VERSION_BYTES;
static const uint8_t g_app_i2c_sw_version[] = APP_I2C_SW_VERSION_BYTES;

const app_i2c_command_descriptor_t g_app_i2c_commands[] =
{
    {
        APP_I2C_REG_ADDR_HW_VERSION,
        g_app_i2c_hw_version,
        (uint8_t)(sizeof g_app_i2c_hw_version),
        NULL
    },
    {
        APP_I2C_REG_ADDR_SW_VERSION,
        g_app_i2c_sw_version,
        (uint8_t)(sizeof g_app_i2c_sw_version),
        NULL
    }
};

const size_t g_app_i2c_command_count = sizeof g_app_i2c_commands / sizeof g_app_i2c_commands[0];

const app_i2c_command_descriptor_t *APP_I2C_FindCommand(uint8_t reg_address)
{
    const app_i2c_command_descriptor_t *entry = NULL;

    for (size_t i = 0U; i < g_app_i2c_command_count; ++i)
    {
        if (g_app_i2c_commands[i].reg_address == reg_address)
        {
            entry = &g_app_i2c_commands[i];
            break;
        }
    }

    return entry;
}