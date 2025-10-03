#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/* Forward declaration for board-specific pin identifiers */
typedef enum hal_gpio_pin_id_t hal_gpio_pin_id_t;

typedef enum
{
    HAL_GPIO_MODE_INPUT = 0,
    HAL_GPIO_MODE_INPUT_PULLUP,
    HAL_GPIO_MODE_OUTPUT
} hal_gpio_mode_t;

typedef enum
{
    HAL_GPIO_LEVEL_LOW = 0,
    HAL_GPIO_LEVEL_HIGH = 1
} hal_gpio_level_t;

typedef struct
{
    volatile uint8_t *port_reg;
    volatile uint8_t *direction_reg;
    volatile uint8_t *pullup_reg;
    volatile const uint8_t *input_reg;
    uint8_t mask;
    hal_gpio_mode_t default_mode;
    hal_gpio_level_t default_level;
    bool inverted;
} hal_gpio_pin_config_t;

void HAL_GPIO_Init(void);

bool HAL_GPIO_IsValid(hal_gpio_pin_id_t pin);

bool HAL_GPIO_SetMode(hal_gpio_pin_id_t pin, hal_gpio_mode_t mode);

bool HAL_GPIO_Write(hal_gpio_pin_id_t pin, hal_gpio_level_t level);

bool HAL_GPIO_Read(hal_gpio_pin_id_t pin, hal_gpio_level_t *level);

bool HAL_GPIO_Toggle(hal_gpio_pin_id_t pin);

uint8_t HAL_GPIO_PinCount(void);

#endif /* HAL_GPIO_H */
