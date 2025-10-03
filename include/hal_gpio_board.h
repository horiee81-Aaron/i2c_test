#ifndef HAL_GPIO_BOARD_H
#define HAL_GPIO_BOARD_H

#include "hal_gpio.h"

/*
 * Define the board-specific GPIO layout by editing HAL_GPIO_PIN_TABLE below.
 * Template:
 *   #define HAL_GPIO_PIN_TABLE(ENTRY) \
 *       ENTRY(HAL_GPIO_PIN_FOO, &PX, &PMX, &PUX, NULL, (uint8_t)(1U << N), \
 *             HAL_GPIO_MODE_OUTPUT, HAL_GPIO_LEVEL_LOW, false)
 */
#ifndef HAL_GPIO_PIN_TABLE
#define HAL_GPIO_PIN_TABLE(_ENTRY) /* Populate with board pins */
#endif

#endif /* HAL_GPIO_BOARD_H */
