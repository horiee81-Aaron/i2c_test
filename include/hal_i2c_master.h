#ifndef HAL_I2C_MASTER_H
#define HAL_I2C_MASTER_H

#include <stdbool.h>
#include <stdint.h>

#define HAL_I2C_M_EEPROM_DEVICE_ADDRESS8  (0xA0U)
#define HAL_I2C_M_EEPROM_DEVICE_ADDRESS7  (HAL_I2C_M_EEPROM_DEVICE_ADDRESS8 >> 1)
#define HAL_I2C_M_EEPROM_PAGE_SIZE        (16U)

bool HAL_I2C_M_Write(uint8_t address, const uint8_t *data, uint16_t length);
bool HAL_I2C_M_Read(uint8_t address, uint8_t *data, uint16_t length);
bool HAL_I2C_M_WriteRead(uint8_t address,
                         const uint8_t *tx_data,
                         uint16_t tx_length,
                         uint8_t *rx_data,
                         uint16_t rx_length);

bool HAL_I2C_M_EEPROM_Write(uint16_t memory_address, const uint8_t *data, uint16_t length);
bool HAL_I2C_M_EEPROM_Read(uint16_t memory_address, uint8_t *data, uint16_t length);

#endif /* HAL_I2C_MASTER_H */
