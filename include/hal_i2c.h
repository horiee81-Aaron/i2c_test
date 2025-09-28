#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdbool.h>
#include <stdint.h>

#define HAL_I2C_MESSAGE_MAX_BYTES   (32U)
#define HAL_I2C_RING_CAPACITY       (8U)
#define HAL_I2C_SLAVE_TIMEOUT_MS    (5U)

typedef enum
{
    HAL_I2C_ERR_NONE = 0,
    HAL_I2C_ERR_OVERRUN,
    HAL_I2C_ERR_TIMEOUT,
    HAL_I2C_ERR_NACK,
    HAL_I2C_ERR_BUS_ERROR,
    HAL_I2C_ERR_ARBITRATION_LOST,
    HAL_I2C_ERR_LINE_STUCK,
    HAL_I2C_ERR_FRAME
} hal_i2c_error_t;

typedef struct
{
    uint8_t  data[HAL_I2C_MESSAGE_MAX_BYTES];
    uint8_t  length;
    uint8_t  hw_status_flags;
    uint32_t timestamp_ms;
} hal_i2c_message_t;

typedef struct
{
    hal_i2c_error_t code;
    uint8_t         hw_status_flags;
    bool            message_dropped;
    uint32_t        timestamp_ms;
} hal_i2c_error_context_t;

typedef void (*hal_i2c_error_callback_t)(const hal_i2c_error_context_t *context);

void HAL_I2C_Init(hal_i2c_error_callback_t error_cb);
void HAL_I2C_ResetSlave(void);

bool HAL_I2C_PopMessage(hal_i2c_message_t *message);

bool HAL_I2C_SetSlaveResponse(const uint8_t *payload, uint8_t length);
bool HAL_I2C_GetSlaveResponse(const uint8_t **payload, uint8_t *length);
void HAL_I2C_ClearSlaveResponse(void);


void HAL_I2C_OnStartCondition(uint8_t hw_status_flags);
void HAL_I2C_OnByteReceived(uint8_t data);
void HAL_I2C_OnStopCondition(uint8_t hw_status_flags);
void HAL_I2C_OnHardwareError(uint8_t hw_status_flags);

void HAL_I2C_Tick1ms(void);

#endif /* HAL_I2C_H */

