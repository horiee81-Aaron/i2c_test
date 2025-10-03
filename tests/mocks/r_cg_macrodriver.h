#ifndef R_CG_MACRODRIVER_H
#define R_CG_MACRODRIVER_H

#include <stdint.h>

extern volatile uint8_t P6;
extern volatile uint8_t PM6;
extern volatile uint8_t PMC6;
extern volatile uint8_t PU6;

static inline void R_Config_NOP(void)
{
    /* No operation in host environment */
}

static inline void R_Systeminit(void)
{
    /* Stubbed for host testing */
}

static inline void __enable_interrupt(void)
{
    /* Stubbed for host testing */
}

static inline uint8_t R_WDT_Restart(void)
{
    return 0U;
}

#endif /* R_CG_MACRODRIVER_H */
