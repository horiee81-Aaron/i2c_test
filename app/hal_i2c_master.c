#include "hal_i2c_master.h"

#include "r_cg_macrodriver.h"
#include "r_config_iica0.h"

#include <stddef.h>
#include <string.h>

#define HAL_I2C_MASTER_SCL_MASK   (uint8_t)(1U << 1)
#define HAL_I2C_MASTER_SDA_MASK   (uint8_t)(1U << 0)
#define HAL_I2C_MASTER_PINS_MASK  (uint8_t)(HAL_I2C_MASTER_SCL_MASK | HAL_I2C_MASTER_SDA_MASK)
#define HAL_I2C_MASTER_DELAY_CYCLES UINT16_C(200)
#define HAL_I2C_MASTER_MAX_ACK_POLL_ATTEMPTS (100U)

typedef struct
{
    uint8_t p6;
    uint8_t pm6;
#if defined(PMC6)
    uint8_t pmc6;
#endif
#if defined(PU6)
    uint8_t pu6;
#endif
} hal_i2c_master_port_snapshot_t;

static void hal_i2c_master_delay(uint16_t cycles)
{
    volatile uint16_t counter = cycles;

    while (counter-- != 0U)
    {
        /* Busy wait to honour I2C timing */
    }
}

static void hal_i2c_master_release_scl(void)
{
    PM6 |= HAL_I2C_MASTER_SCL_MASK;
}

static void hal_i2c_master_release_sda(void)
{
    PM6 |= HAL_I2C_MASTER_SDA_MASK;
}

static void hal_i2c_master_drive_scl_low(void)
{
    P6 &= (uint8_t)(~HAL_I2C_MASTER_SCL_MASK);
    PM6 &= (uint8_t)(~HAL_I2C_MASTER_SCL_MASK);
}

static void hal_i2c_master_drive_sda_low(void)
{
    P6 &= (uint8_t)(~HAL_I2C_MASTER_SDA_MASK);
    PM6 &= (uint8_t)(~HAL_I2C_MASTER_SDA_MASK);
}

static bool hal_i2c_master_read_sda(void)
{
    return ((P6 & HAL_I2C_MASTER_SDA_MASK) != 0U);
}

static void hal_i2c_master_start_condition(void)
{
    hal_i2c_master_release_sda();
    hal_i2c_master_release_scl();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);

    hal_i2c_master_drive_sda_low();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_drive_scl_low();
}

static void hal_i2c_master_stop_condition(void)
{
    hal_i2c_master_drive_sda_low();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_release_scl();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_release_sda();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
}

static void hal_i2c_master_write_bit(bool bit)
{
    if (bit)
    {
        hal_i2c_master_release_sda();
    }
    else
    {
        hal_i2c_master_drive_sda_low();
    }

    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_release_scl();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_drive_scl_low();
}

static bool hal_i2c_master_read_bit(void)
{
    hal_i2c_master_release_sda();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    hal_i2c_master_release_scl();
    hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
    const bool high = hal_i2c_master_read_sda();
    hal_i2c_master_drive_scl_low();

    return high;
}

static bool hal_i2c_master_write_byte(uint8_t value)
{
    for (uint8_t mask = UINT8_C(0x80); mask != 0U; mask >>= 1)
    {
        hal_i2c_master_write_bit((value & mask) != 0U);
    }

    const bool ack = (hal_i2c_master_read_bit() == false);

    return ack;
}

static uint8_t hal_i2c_master_read_byte(bool ack)
{
    uint8_t value = 0U;

    for (uint8_t mask = UINT8_C(0x80); mask != 0U; mask >>= 1)
    {
        if (hal_i2c_master_read_bit())
        {
            value |= mask;
        }
    }

    hal_i2c_master_write_bit(!ack);

    return value;
}

static uint8_t hal_i2c_master_normalize_address(uint8_t address)
{
    if ((address & UINT8_C(0x80)) != 0U)
    {
        address >>= 1;
    }

    return (uint8_t)(address & UINT8_C(0x7F));
}

static bool hal_i2c_master_acquire_bus(hal_i2c_master_port_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return false;
    }

    R_Config_IICA0_Stop();

    snapshot->p6  = P6;
    snapshot->pm6 = PM6;
#if defined(PMC6)
    snapshot->pmc6 = PMC6;
#endif
#if defined(PU6)
    snapshot->pu6 = PU6;
#endif

    P6  |= HAL_I2C_MASTER_PINS_MASK;
    PM6 |= HAL_I2C_MASTER_PINS_MASK;
#if defined(PU6)
    PU6 |= HAL_I2C_MASTER_PINS_MASK;
#endif
#if defined(PMC6)
    PMC6 &= (uint8_t)(~HAL_I2C_MASTER_PINS_MASK);
#endif

    return true;
}

static void hal_i2c_master_release_bus(const hal_i2c_master_port_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

#if defined(PU6)
    PU6 = (uint8_t)((PU6 & (uint8_t)(~HAL_I2C_MASTER_PINS_MASK)) | (snapshot->pu6 & HAL_I2C_MASTER_PINS_MASK));
#endif
#if defined(PMC6)
    PMC6 = (uint8_t)((PMC6 & (uint8_t)(~HAL_I2C_MASTER_PINS_MASK)) | (snapshot->pmc6 & HAL_I2C_MASTER_PINS_MASK));
#endif
    P6  = (uint8_t)((P6 & (uint8_t)(~HAL_I2C_MASTER_PINS_MASK)) | (snapshot->p6 & HAL_I2C_MASTER_PINS_MASK));
    PM6 = (uint8_t)((PM6 & (uint8_t)(~HAL_I2C_MASTER_PINS_MASK)) | (snapshot->pm6 & HAL_I2C_MASTER_PINS_MASK));

    R_Config_IICA0_Create();
    R_Config_IICA0_Start();
    R_Config_IICA0_SlaveReceiveStart();
}

static bool hal_i2c_master_poll_ack(uint8_t address)
{
    hal_i2c_master_port_snapshot_t snapshot;

    if (!hal_i2c_master_acquire_bus(&snapshot))
    {
        return false;
    }

    const uint8_t write_address = (uint8_t)((hal_i2c_master_normalize_address(address) << 1) | 0U);
    bool acknowledged = false;

    hal_i2c_master_start_condition();
    acknowledged = hal_i2c_master_write_byte(write_address);
    hal_i2c_master_stop_condition();

    hal_i2c_master_release_bus(&snapshot);

    return acknowledged;
}

bool HAL_I2C_M_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return false;
    }

    return HAL_I2C_M_WriteRead(address, data, length, NULL, 0U);
}

bool HAL_I2C_M_Read(uint8_t address, uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return false;
    }

    return HAL_I2C_M_WriteRead(address, NULL, 0U, data, length);
}

bool HAL_I2C_M_WriteRead(uint8_t address,
                              const uint8_t *tx_data,
                              uint16_t tx_length,
                              uint8_t *rx_data,
                              uint16_t rx_length)
{
    if ((tx_length == 0U) && (rx_length == 0U))
    {
        return false;
    }

    if ((tx_length > 0U) && (tx_data == NULL))
    {
        return false;
    }

    if ((rx_length > 0U) && (rx_data == NULL))
    {
        return false;
    }

    hal_i2c_master_port_snapshot_t snapshot;

    if (!hal_i2c_master_acquire_bus(&snapshot))
    {
        return false;
    }

    const uint8_t write_address = (uint8_t)((hal_i2c_master_normalize_address(address) << 1) | 0U);
    const uint8_t read_address  = (uint8_t)((hal_i2c_master_normalize_address(address) << 1) | 1U);

    bool success = true;

    if (tx_length > 0U)
    {
        hal_i2c_master_start_condition();
        if (!hal_i2c_master_write_byte(write_address))
        {
            success = false;
        }
        else
        {
            for (uint16_t index = 0U; index < tx_length; index++)
            {
                if (!hal_i2c_master_write_byte(tx_data[index]))
                {
                    success = false;
                    break;
                }
            }
        }
    }

    if (success && (rx_length > 0U))
    {
        hal_i2c_master_start_condition();
        if (!hal_i2c_master_write_byte(read_address))
        {
            success = false;
        }
        else
        {
            for (uint16_t index = 0U; index < rx_length; index++)
            {
                const bool acknowledge = (index + 1U) < rx_length;
                rx_data[index] = hal_i2c_master_read_byte(acknowledge);
            }
        }
    }

    hal_i2c_master_stop_condition();
    hal_i2c_master_release_bus(&snapshot);

    return success;
}

static bool hal_i2c_master_eeprom_wait_ready(void)
{
    uint32_t attempts = 0U;
    bool ready = false;

    while ((attempts < HAL_I2C_MASTER_MAX_ACK_POLL_ATTEMPTS) && !ready)
    {
        ready = hal_i2c_master_poll_ack(HAL_I2C_M_EEPROM_DEVICE_ADDRESS7);
        if (!ready)
        {
            hal_i2c_master_delay(HAL_I2C_MASTER_DELAY_CYCLES);
        }
        attempts++;
    }

    return ready;
}

bool HAL_I2C_M_EEPROM_Write(uint16_t memory_address, const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return false;
    }

    uint16_t remaining = length;
    uint16_t offset = 0U;
    bool success = true;
    uint8_t payload[HAL_I2C_M_EEPROM_PAGE_SIZE + 2U];

    while ((remaining > 0U) && success)
    {
        const uint16_t current_address = (uint16_t)(memory_address + offset);
        const uint16_t page_offset = (uint16_t)(current_address % HAL_I2C_M_EEPROM_PAGE_SIZE);
        uint16_t chunk = (uint16_t)(HAL_I2C_M_EEPROM_PAGE_SIZE - page_offset);

        if (chunk > remaining)
        {
            chunk = remaining;
        }

        payload[0] = (uint8_t)(current_address >> 8);
        payload[1] = (uint8_t)(current_address & 0xFFU);
        (void)memcpy(&payload[2], &data[offset], chunk);

        success = HAL_I2C_M_Write(HAL_I2C_M_EEPROM_DEVICE_ADDRESS7, payload, (uint16_t)(chunk + 2U));
        if (!success)
        {
            break;
        }

        success = hal_i2c_master_eeprom_wait_ready();
        if (!success)
        {
            break;
        }

        offset += chunk;
        remaining = (uint16_t)(remaining - chunk);
    }

    return success;
}

bool HAL_I2C_M_EEPROM_Read(uint16_t memory_address, uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return false;
    }

    uint8_t address_bytes[2];

    address_bytes[0] = (uint8_t)(memory_address >> 8);
    address_bytes[1] = (uint8_t)(memory_address & 0xFFU);

    return HAL_I2C_M_WriteRead(HAL_I2C_M_EEPROM_DEVICE_ADDRESS7,
                                    address_bytes,
                                    (uint16_t)2U,
                                    data,
                                    length);
}
