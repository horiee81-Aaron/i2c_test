#ifndef HAL_GPIO_PINMAP_H
#define HAL_GPIO_PINMAP_H

#include "hal_gpio.h"

/*
 * Define HAL_GPIO_PIN_TABLE(ENTRY) before including this header to provide
 * the board-specific mapping between logical pins and hardware registers.
 * Each entry must expand to:
 *   ENTRY(name, port_reg, direction_reg, pullup_reg, input_reg, mask,
 *         default_mode, default_level, inverted)
 *
 * Example:
 *   #define HAL_GPIO_PIN_TABLE(ENTRY) \
 *       ENTRY(HAL_GPIO_PIN_STATUS_LED, &P7, &PM7, &PU7, NULL, (uint8_t)(1U << 0), \
 *             HAL_GPIO_MODE_OUTPUT, HAL_GPIO_LEVEL_LOW, false)
 */
#ifndef HAL_GPIO_PIN_TABLE
#define HAL_GPIO_PIN_TABLE(_ENTRY)
#endif

enum hal_gpio_pin_id_t
{
#define HAL_GPIO_DECLARE_ENUM(name, port_reg, direction_reg, pullup_reg, input_reg, mask, default_mode, default_level, inverted) name,
    HAL_GPIO_PIN_TABLE(HAL_GPIO_DECLARE_ENUM)
#undef HAL_GPIO_DECLARE_ENUM
    HAL_GPIO_PIN_COUNT
};

extern const hal_gpio_pin_config_t g_hal_gpio_pinmap[];
extern const uint8_t g_hal_gpio_pinmap_count;

#endif /* HAL_GPIO_PINMAP_H */
