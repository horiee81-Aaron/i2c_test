#ifndef R_CONFIG_IICA0_H
#define R_CONFIG_IICA0_H

#include <stdint.h>

#define R_IICA0_STATUS_BUS_ERROR        (1U << 0)
#define R_IICA0_STATUS_ARBITRATION_LOST (1U << 1)
#define R_IICA0_STATUS_OVERRUN          (1U << 2)
#define R_IICA0_STATUS_NACK             (1U << 3)
#define R_IICA0_STATUS_LINE_STUCK       (1U << 4)
#define R_IICA0_STATUS_FRAME_ERROR      (1U << 5)

void R_Config_IICA0_ResetBusLines(void);
void R_Config_IICA0_Stop(void);
void R_Config_IICA0_Create(void);
void R_Config_IICA0_Start(void);
void R_Config_IICA0_SlaveReceiveStart(void);

#endif /* R_CONFIG_IICA0_H */
