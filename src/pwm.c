#include <msp430.h>
#include "pwm.h"

void pwm_init(void)
{
    /* Clear Timer B1. */
    TB1CTL = TBCLR;

    /* Expansion divider /8. */
    TB1EX0 = TBIDEX_7;

    /* CCR0 defines period; CCR1 uses reset/set output mode. */
    TB1CCTL0 = 0;
    TB1CCTL1 = OUTMOD_7;

    /*
     * SMCLK / 8 / 8 = 375 kHz when SMCLK = 24 MHz.
     * 20 ms * 375,000 counts/s = 7,500 counts.
     */
    TB1CCR0 = 7500U;
    TB1CCR1 = 0U;

    /* SMCLK, /8, up mode. */
    TB1CTL = TBSSEL__SMCLK | ID__8 | MC__UP;

    /* P2.0 -> TB1.1 PWM output. */
    P2DIR |= BIT0;
    P2SEL0 |= BIT0;
    P2SEL1 &= ~BIT0;
}

void motor_stop(void)
{
    TB1CCR1 = 0U;
}

void motor_clockwise(void)
{
    TB1CCR1 = 525U;
}

void motor_counterclockwise(void)
{
    TB1CCR1 = 750U;
}
