#ifndef MOCK_R_CONFIG_IICA0_H
#define MOCK_R_CONFIG_IICA0_H

#include <stdint.h>

typedef struct
{
    uint32_t reset_bus_lines_calls;
    uint32_t stop_calls;
    uint32_t create_calls;
    uint32_t start_calls;
    uint32_t slave_receive_start_calls;
} mock_r_config_iica0_state_t;

void MOCK_R_Config_IICA0_Reset(void);
const mock_r_config_iica0_state_t *MOCK_R_Config_IICA0_GetState(void);

#endif /* MOCK_R_CONFIG_IICA0_H */
