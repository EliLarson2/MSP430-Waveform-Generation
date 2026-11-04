#include <msp430.h>
#include "sac.h"

void sac_init(void)
{
    /*
     * SACEN = 1: enable SAC
     * OAPM = 0: high-speed/high-power mode
     * OAPEN = 1: enable op-amp
     * NMUXEN = 1: connect negative-input mux
     * NSEL = 01: PGA source selected
     * PMUXEN = 1: connect positive-input mux
     * PSEL = 01: positive input driven by the 12-bit reference DAC
     */
    SAC3OA |= (1U << 10)
            | (0U << 9)
            | (1U << 8)
            | (1U << 7)
            | (1U << 4)
            | (1U << 3)
            | (1U << 0);

    /* MSEL = 01: buffer/voltage-follower mode. */
    SAC3PGA = (1U << 0);

    /* DACEN = 1; DAC loads when SAC3DAT is written. */
    SAC3DAC = (1U << 0);

    /* Route OA3O to P3.5. */
    P3SEL0 |= BIT5;
    P3SEL1 |= BIT5;

    SAC3DAT = 0;
}

void sac_set_dac(uint16_t value)
{
    if (value > 4095U)
    {
        value = 4095U;
    }

    SAC3DAT = value;
}

void sac_set_zero(void)
{
    sac_set_dac(0U);
}

void sac_set_full_scale(void)
{
    sac_set_dac(4095U);
}
