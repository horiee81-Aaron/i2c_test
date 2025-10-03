#include "hal_gpio.h"
#include "hal_gpio_board.h"
#include "hal_gpio_pinmap.h"

#include <stddef.h>

#if HAL_GPIO_PIN_COUNT > 0
const hal_gpio_pin_config_t g_hal_gpio_pinmap[HAL_GPIO_PIN_COUNT] =
{
#define HAL_GPIO_DEFINE_ENTRY(name, port_reg, direction_reg, pullup_reg, input_reg, mask, default_mode, default_level, inverted) \
    [name] = { (port_reg), (direction_reg), (pullup_reg), (input_reg), (uint8_t)(mask), (default_mode), (default_level), (inverted) },
    HAL_GPIO_PIN_TABLE(HAL_GPIO_DEFINE_ENTRY)
#undef HAL_GPIO_DEFINE_ENTRY
};
#else
const hal_gpio_pin_config_t g_hal_gpio_pinmap[1] =
{
    { NULL, NULL, NULL, NULL, 0U, HAL_GPIO_MODE_INPUT, HAL_GPIO_LEVEL_LOW, false }
};
#endif

const uint8_t g_hal_gpio_pinmap_count = (uint8_t)HAL_GPIO_PIN_COUNT;

static const hal_gpio_pin_config_t *hal_gpio_get_config(hal_gpio_pin_id_t pin);
static void hal_gpio_set_pullup(const hal_gpio_pin_config_t *cfg, bool enable);
static bool hal_gpio_set_mode_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_mode_t mode);
static bool hal_gpio_write_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_level_t level);
static bool hal_gpio_read_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_level_t *level);

void HAL_GPIO_Init(void)
{
    for (uint8_t idx = 0U; idx < g_hal_gpio_pinmap_count; ++idx)
    {
        const hal_gpio_pin_config_t *cfg = &g_hal_gpio_pinmap[idx];

        if ((cfg->port_reg == NULL) || (cfg->direction_reg == NULL))
        {
            continue;
        }

        if (cfg->default_mode == HAL_GPIO_MODE_OUTPUT)
        {
            (void)hal_gpio_write_raw(cfg, cfg->default_level);
        }

        (void)hal_gpio_set_mode_raw(cfg, cfg->default_mode);
    }
}

bool HAL_GPIO_IsValid(hal_gpio_pin_id_t pin)
{
    bool valid = false;

    if ((int)pin >= 0)
    {
        if ((uint8_t)pin < g_hal_gpio_pinmap_count)
        {
            const hal_gpio_pin_config_t *cfg = &g_hal_gpio_pinmap[(uint8_t)pin];
            valid = (cfg->port_reg != NULL) && (cfg->direction_reg != NULL);
        }
    }

    return valid;
}

bool HAL_GPIO_SetMode(hal_gpio_pin_id_t pin, hal_gpio_mode_t mode)
{
    const hal_gpio_pin_config_t *cfg = hal_gpio_get_config(pin);

    return hal_gpio_set_mode_raw(cfg, mode);
}

bool HAL_GPIO_Write(hal_gpio_pin_id_t pin, hal_gpio_level_t level)
{
    const hal_gpio_pin_config_t *cfg = hal_gpio_get_config(pin);

    return hal_gpio_write_raw(cfg, level);
}

bool HAL_GPIO_Read(hal_gpio_pin_id_t pin, hal_gpio_level_t *level)
{
    const hal_gpio_pin_config_t *cfg = hal_gpio_get_config(pin);

    return hal_gpio_read_raw(cfg, level);
}

bool HAL_GPIO_Toggle(hal_gpio_pin_id_t pin)
{
    hal_gpio_level_t current = HAL_GPIO_LEVEL_LOW;

    if (!HAL_GPIO_Read(pin, &current))
    {
        return false;
    }

    if (current == HAL_GPIO_LEVEL_HIGH)
    {
        return HAL_GPIO_Write(pin, HAL_GPIO_LEVEL_LOW);
    }

    return HAL_GPIO_Write(pin, HAL_GPIO_LEVEL_HIGH);
}

uint8_t HAL_GPIO_PinCount(void)
{
    return g_hal_gpio_pinmap_count;
}

static const hal_gpio_pin_config_t *hal_gpio_get_config(hal_gpio_pin_id_t pin)
{
    if (HAL_GPIO_IsValid(pin))
    {
        return &g_hal_gpio_pinmap[(uint8_t)pin];
    }

    return NULL;
}

static void hal_gpio_set_pullup(const hal_gpio_pin_config_t *cfg, bool enable)
{
    if ((cfg != NULL) && (cfg->pullup_reg != NULL))
    {
        if (enable)
        {
            *cfg->pullup_reg |= cfg->mask;
        }
        else
        {
            *cfg->pullup_reg &= (uint8_t)(~cfg->mask);
        }
    }
}

static bool hal_gpio_set_mode_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_mode_t mode)
{
    if (cfg == NULL)
    {
        return false;
    }

    if (cfg->direction_reg == NULL)
    {
        return false;
    }

    switch (mode)
    {
        case HAL_GPIO_MODE_OUTPUT:
            *cfg->direction_reg &= (uint8_t)(~cfg->mask);
            hal_gpio_set_pullup(cfg, false);
            break;
        case HAL_GPIO_MODE_INPUT:
            *cfg->direction_reg |= cfg->mask;
            hal_gpio_set_pullup(cfg, false);
            break;
        case HAL_GPIO_MODE_INPUT_PULLUP:
            *cfg->direction_reg |= cfg->mask;
            hal_gpio_set_pullup(cfg, true);
            break;
        default:
            return false;
    }

    return true;
}

static bool hal_gpio_write_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_level_t level)
{
    if ((cfg == NULL) || (cfg->port_reg == NULL))
    {
        return false;
    }

    bool drive_high = (level == HAL_GPIO_LEVEL_HIGH);

    if (cfg->inverted)
    {
        drive_high = !drive_high;
    }

    uint8_t port_value = *cfg->port_reg;

    if (drive_high)
    {
        port_value |= cfg->mask;
    }
    else
    {
        port_value &= (uint8_t)(~cfg->mask);
    }

    *cfg->port_reg = port_value;

    return true;
}

static bool hal_gpio_read_raw(const hal_gpio_pin_config_t *cfg, hal_gpio_level_t *level)
{
    if ((cfg == NULL) || (level == NULL))
    {
        return false;
    }

    const volatile uint8_t *input_reg = cfg->input_reg;

    if (input_reg == NULL)
    {
        input_reg = cfg->port_reg;
    }

    if (input_reg == NULL)
    {
        return false;
    }

    bool is_high = ((*input_reg & cfg->mask) != 0U);

    if (cfg->inverted)
    {
        is_high = !is_high;
    }

    *level = is_high ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;

    return true;
}
