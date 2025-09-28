#include "hal_i2c.h"
#include "hal_scheduler.h"
#include "r_config_iica0.h"

#include <stddef.h>
#include <string.h>

#if defined(R_IICA0_STATUS_BUS_ERROR) && !defined(R_CONFIG_IICA0_STATUS_BUS_ERROR)
#define R_CONFIG_IICA0_STATUS_BUS_ERROR        (R_IICA0_STATUS_BUS_ERROR)
#endif
#if defined(R_IICA0_STATUS_ARBITRATION_LOST) && !defined(R_CONFIG_IICA0_STATUS_ARBITRATION_LOST)
#define R_CONFIG_IICA0_STATUS_ARBITRATION_LOST (R_IICA0_STATUS_ARBITRATION_LOST)
#endif
#if defined(R_IICA0_STATUS_OVERRUN) && !defined(R_CONFIG_IICA0_STATUS_OVERRUN)
#define R_CONFIG_IICA0_STATUS_OVERRUN          (R_IICA0_STATUS_OVERRUN)
#endif
#if defined(R_IICA0_STATUS_NACK) && !defined(R_CONFIG_IICA0_STATUS_NACK)
#define R_CONFIG_IICA0_STATUS_NACK             (R_IICA0_STATUS_NACK)
#endif
#if defined(R_IICA0_STATUS_LINE_STUCK) && !defined(R_CONFIG_IICA0_STATUS_LINE_STUCK)
#define R_CONFIG_IICA0_STATUS_LINE_STUCK       (R_IICA0_STATUS_LINE_STUCK)
#endif
#if defined(R_IICA0_STATUS_FRAME_ERROR) && !defined(R_CONFIG_IICA0_STATUS_FRAME_ERROR)
#define R_CONFIG_IICA0_STATUS_FRAME_ERROR      (R_IICA0_STATUS_FRAME_ERROR)
#endif

#ifndef R_CONFIG_IICA0_STATUS_BUS_ERROR
#define R_CONFIG_IICA0_STATUS_BUS_ERROR        (0U)
#endif
#ifndef R_CONFIG_IICA0_STATUS_ARBITRATION_LOST
#define R_CONFIG_IICA0_STATUS_ARBITRATION_LOST (0U)
#endif
#ifndef R_CONFIG_IICA0_STATUS_OVERRUN
#define R_CONFIG_IICA0_STATUS_OVERRUN          (0U)
#endif
#ifndef R_CONFIG_IICA0_STATUS_NACK
#define R_CONFIG_IICA0_STATUS_NACK             (0U)
#endif
#ifndef R_CONFIG_IICA0_STATUS_LINE_STUCK
#define R_CONFIG_IICA0_STATUS_LINE_STUCK       (0U)
#endif
#ifndef R_CONFIG_IICA0_STATUS_FRAME_ERROR
#define R_CONFIG_IICA0_STATUS_FRAME_ERROR      (0U)
#endif

typedef struct
{
    hal_i2c_message_t queue[HAL_I2C_RING_CAPACITY];
    volatile uint8_t  head;
    volatile uint8_t  tail;
    volatile uint8_t  count;

    hal_i2c_message_t current;
    bool              receiving;
    uint16_t          in_frame_ticks;

    struct
    {
        uint8_t data[HAL_I2C_MESSAGE_MAX_BYTES];
        uint8_t length;
        uint8_t index;
        bool    pending;
    } response;

    hal_i2c_error_callback_t error_cb;
} hal_i2c_context_t;

static hal_i2c_context_t g_i2c_ctx = {0};

static void hal_i2c_rearm_hardware(void);
static void hal_i2c_reset_current_message(void);
static void hal_i2c_clear_response(void);
static hal_i2c_error_t hal_i2c_map_error(uint8_t hw_flags);
static void hal_i2c_report_error(hal_i2c_error_t code, uint8_t hw_flags, bool dropped);

static void hal_i2c_rearm_hardware(void)
{
    R_Config_IICA0_Stop();
    R_Config_IICA0_Create();
    R_Config_IICA0_Start();
    R_Config_IICA0_SlaveReceiveStart();
}

static void hal_i2c_reset_current_message(void)
{
    g_i2c_ctx.receiving      = false;
    g_i2c_ctx.in_frame_ticks = 0U;
    g_i2c_ctx.current.length = 0U;
    g_i2c_ctx.current.hw_status_flags = 0U;
    g_i2c_ctx.current.timestamp_ms    = UINT32_C(0);
}

static void hal_i2c_clear_response(void)
{
    g_i2c_ctx.response.length = 0U;
    g_i2c_ctx.response.index  = 0U;
    g_i2c_ctx.response.pending = false;
    (void)memset(g_i2c_ctx.response.data, 0, sizeof g_i2c_ctx.response.data);
}

static hal_i2c_error_t hal_i2c_map_error(uint8_t hw_flags)
{
    hal_i2c_error_t status = HAL_I2C_ERR_NONE;

    if ((hw_flags & R_CONFIG_IICA0_STATUS_BUS_ERROR) != 0U)
    {
        status = HAL_I2C_ERR_BUS_ERROR;
    }
    else if ((hw_flags & R_CONFIG_IICA0_STATUS_ARBITRATION_LOST) != 0U)
    {
        status = HAL_I2C_ERR_ARBITRATION_LOST;
    }
    else if ((hw_flags & R_CONFIG_IICA0_STATUS_OVERRUN) != 0U)
    {
        status = HAL_I2C_ERR_OVERRUN;
    }
    else if ((hw_flags & R_CONFIG_IICA0_STATUS_NACK) != 0U)
    {
        status = HAL_I2C_ERR_NACK;
    }
    else if ((hw_flags & R_CONFIG_IICA0_STATUS_LINE_STUCK) != 0U)
    {
        status = HAL_I2C_ERR_LINE_STUCK;
    }
    else if ((hw_flags & R_CONFIG_IICA0_STATUS_FRAME_ERROR) != 0U)
    {
        status = HAL_I2C_ERR_FRAME;
    }
    else
    {
        /* No action required */
    }

    return status;
}

static void hal_i2c_report_error(hal_i2c_error_t code, uint8_t hw_flags, bool dropped)
{
    if (g_i2c_ctx.error_cb != NULL)
    {
        hal_i2c_error_context_t context;

        context.code            = code;
        context.hw_status_flags = hw_flags;
        context.message_dropped = dropped;
        context.timestamp_ms    = HAL_SCHED_GetUptimeMs();

        g_i2c_ctx.error_cb(&context);
    }
    else
    {
        /* No action required */
    }
}

void HAL_I2C_Init(hal_i2c_error_callback_t error_cb)
{
    g_i2c_ctx.head           = 0U;
    g_i2c_ctx.tail           = 0U;
    g_i2c_ctx.count          = 0U;
    g_i2c_ctx.error_cb       = error_cb;

    hal_i2c_clear_response();
    hal_i2c_reset_current_message();
    hal_i2c_rearm_hardware();
}

void HAL_I2C_ResetSlave(void)
{
    hal_i2c_rearm_hardware();
    hal_i2c_clear_response();
    hal_i2c_reset_current_message();
}

bool HAL_I2C_PopMessage(hal_i2c_message_t *message)
{
    bool has_message = false;

    if ((message != NULL) && (g_i2c_ctx.count > 0U))
    {
        *message = g_i2c_ctx.queue[g_i2c_ctx.tail];
        g_i2c_ctx.tail = (uint8_t)((g_i2c_ctx.tail + 1U) % HAL_I2C_RING_CAPACITY);
        g_i2c_ctx.count--;
        has_message = true;
    }
    else
    {
        /* No action required */
    }

    return has_message;
}

bool HAL_I2C_SetSlaveResponse(const uint8_t *payload, uint8_t length)
{
    bool success = false;

    if (length == 0U)
    {
        hal_i2c_clear_response();
        success = true;
    }
    else if ((payload != NULL) && (length <= HAL_I2C_MESSAGE_MAX_BYTES))
    {
        (void)memcpy(g_i2c_ctx.response.data, payload, length);
        g_i2c_ctx.response.length  = length;
        g_i2c_ctx.response.index   = 0U;
        g_i2c_ctx.response.pending = true;
        success = true;
    }
    else
    {
        /* No action required */
    }

    return success;
}

bool HAL_I2C_GetSlaveResponse(const uint8_t **payload, uint8_t *length)
{
    bool has_payload = false;

    if ((payload != NULL) && (length != NULL))
    {
        if (g_i2c_ctx.response.pending != false)
        {
            *payload = g_i2c_ctx.response.data;
            *length  = g_i2c_ctx.response.length;
            has_payload = true;
        }
        else
        {
            *payload = NULL;
            *length  = 0U;
        }
    }
    else
    {
        /* No action required */
    }

    return has_payload;
}

void HAL_I2C_ClearSlaveResponse(void)
{
    hal_i2c_clear_response();
}



void HAL_I2C_OnStartCondition(uint8_t hw_status_flags)
{
    g_i2c_ctx.receiving                = true;
    g_i2c_ctx.current.length           = 0U;
    g_i2c_ctx.current.hw_status_flags  = hw_status_flags;
    g_i2c_ctx.in_frame_ticks           = 0U;
    g_i2c_ctx.current.timestamp_ms     = UINT32_C(0);
}

void HAL_I2C_OnByteReceived(uint8_t data)
{
    if (g_i2c_ctx.receiving != false)
    {
        if (g_i2c_ctx.current.length < HAL_I2C_MESSAGE_MAX_BYTES)
        {
            g_i2c_ctx.current.data[g_i2c_ctx.current.length] = data;
            g_i2c_ctx.current.length++;
            g_i2c_ctx.in_frame_ticks = 0U;
        }
        else
        {
            hal_i2c_report_error(HAL_I2C_ERR_OVERRUN, g_i2c_ctx.current.hw_status_flags, true);
            HAL_I2C_ResetSlave();
        }
    }
    else
    {
        /* No action required */
    }
}

void HAL_I2C_OnStopCondition(uint8_t hw_status_flags)
{
    if (g_i2c_ctx.receiving != false)
    {
        g_i2c_ctx.receiving               = false;
        g_i2c_ctx.current.hw_status_flags  = hw_status_flags;
        g_i2c_ctx.current.timestamp_ms     = HAL_SCHED_GetUptimeMs();

        if (g_i2c_ctx.count < HAL_I2C_RING_CAPACITY)
        {
            g_i2c_ctx.queue[g_i2c_ctx.head] = g_i2c_ctx.current;
            g_i2c_ctx.head = (uint8_t)((g_i2c_ctx.head + 1U) % HAL_I2C_RING_CAPACITY);
            g_i2c_ctx.count++;
        }
        else
        {
            hal_i2c_report_error(HAL_I2C_ERR_OVERRUN, hw_status_flags, true);
        }

        hal_i2c_reset_current_message();
    }
    else
    {
        hal_i2c_report_error(HAL_I2C_ERR_FRAME, hw_status_flags, false);
    }
}

void HAL_I2C_OnHardwareError(uint8_t hw_status_flags)
{
    const hal_i2c_error_t mapped = hal_i2c_map_error(hw_status_flags);

    if (mapped != HAL_I2C_ERR_NONE)
    {
        hal_i2c_report_error(mapped, hw_status_flags, g_i2c_ctx.receiving);
    }
    else
    {
        /* No action required */
    }

    HAL_I2C_ResetSlave();
}

void HAL_I2C_Tick1ms(void)
{
    if (g_i2c_ctx.receiving != false)
    {
        if (g_i2c_ctx.in_frame_ticks < UINT16_MAX)
        {
            g_i2c_ctx.in_frame_ticks++;
        }
        else
        {
            /* No action required */
        }

        if (g_i2c_ctx.in_frame_ticks >= HAL_I2C_SLAVE_TIMEOUT_MS)
        {
            hal_i2c_report_error(HAL_I2C_ERR_TIMEOUT, g_i2c_ctx.current.hw_status_flags, true);
            HAL_I2C_ResetSlave();
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


