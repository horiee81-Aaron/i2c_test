#ifndef APP_I2C_REGISTERS_H
#define APP_I2C_REGISTERS_H

#include <stddef.h>
#include <stdint.h>

#include "hal_i2c.h"

#define APP_I2C_REG_ADDR_HW_VERSION    (0x01U)
#define APP_I2C_REG_ADDR_SW_VERSION    (0x02U)

#define APP_I2C_HW_VERSION_BYTES       { 0x00U, 0x01U }
#define APP_I2C_SW_VERSION_BYTES       { 0x00U, 0x10U }

typedef void (*app_i2c_command_handler_t)(const hal_i2c_message_t *message);

typedef struct
{
    uint8_t                   reg_address;
    const uint8_t            *response;
    uint8_t                   response_length;
    app_i2c_command_handler_t handler;
} app_i2c_command_descriptor_t;

extern const app_i2c_command_descriptor_t g_app_i2c_commands[];
extern const size_t g_app_i2c_command_count;

const app_i2c_command_descriptor_t *APP_I2C_FindCommand(uint8_t reg_address);

#endif /* APP_I2C_REGISTERS_H */