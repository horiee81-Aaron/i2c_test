#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "hal_i2c.h"
#include "app_i2c_registers.h"
#include "hal_scheduler.h"
#include "r_cg_macrodriver.h"
#include "r_config_iica0.h"

static void process_message(const hal_i2c_message_t *message)
{
    HAL_I2C_ClearSlaveResponse();
    if ((message == NULL) || (message->length == 0U))
    {
        return;
    }
    const uint8_t register_address = message->data[0];
    const app_i2c_command_descriptor_t *command = APP_I2C_FindCommand(register_address);
    if (command == NULL)
    {
        return;
    }
    if (command->handler != NULL)
    {
        command->handler(message);
    }
    if ((command->response != NULL) && (command->response_length > 0U))
    {
        (void)HAL_I2C_SetSlaveResponse(command->response, command->response_length);
    }
}

static void Task_ProcessI2C(void)
{
    hal_i2c_message_t message;
    if (HAL_I2C_PopMessage(&message))
    {
        process_message(&message);
    }
}

static void Task_Housekeeping(void)
{
    (void)R_WDT_Restart();
}

static hal_sched_task_t g_tasks[] = {
    { Task_ProcessI2C, UINT32_C(0), UINT16_C(1) },
    { Task_Housekeeping, UINT32_C(0), UINT16_C(10) }
};

static void App_I2C_ErrorHandler(const hal_i2c_error_context_t *context)
{
    if (context == NULL)
    {
        return;
    }
    switch (context->code)
    {
        case HAL_I2C_ERR_BUS_ERROR:
        case HAL_I2C_ERR_LINE_STUCK:
            R_Config_IICA0_ResetBusLines();
            HAL_I2C_ResetSlave();
            break;
        case HAL_I2C_ERR_OVERRUN:
            HAL_I2C_ResetSlave();
            break;
        case HAL_I2C_ERR_TIMEOUT:
        case HAL_I2C_ERR_NACK:
        case HAL_I2C_ERR_ARBITRATION_LOST:
        case HAL_I2C_ERR_FRAME:
        default:
            HAL_I2C_ResetSlave();
            break;
    }
}

int main(void)
{
    R_Systeminit();
    __enable_interrupt();
    HAL_SCHED_Init(UINT16_C(1000));
    const size_t count = sizeof g_tasks / sizeof g_tasks[0];
    if (count <= (size_t)UINT8_MAX)
    {
        HAL_SCHED_RegisterTasks(g_tasks, (uint8_t)count);
    }
    HAL_I2C_Init(App_I2C_ErrorHandler);
    for (;;)
    {
        HAL_SCHED_RunOnce();
        R_Config_NOP();
    }
    return 0;
}
