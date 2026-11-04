#include <msp430.h>
#include <stdint.h>

#include "timer.h"
#include "nco.h"
#include "sac.h"

volatile uint8_t g_nco_enabled = 0U;
volatile uint8_t g_led_enabled = 0U;

static volatile uint16_t g_toggle_counter = 0U;

void timer_init(void)
{
    /* Reset Timer B0. */
    TB0CTL = TBCLR;

    /* Expansion divider /8. */
    TB0EX0 = TBIDEX_7;

    /* Enable CCR0 compare interrupt. */
    TB0CCTL0 = CCIE;

    /*
     * SMCLK / 8 / 8 = 375 kHz when SMCLK = 24 MHz.
     * 1 ms * 375,000 counts/s = 375 counts.
     */
    TB0CCR0 = 375U;

    /* SMCLK, /8, up mode. */
    TB0CTL = TBSSEL__SMCLK | ID__8 | MC__UP;
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TimerB0_ISR(void)
{
    int32_t signed_sample;
    uint16_t dac_sample;

    g_toggle_counter++;

    if (g_led_enabled && g_toggle_counter >= 500U)
    {
        P1OUT ^= BIT0;
        g_toggle_counter = 0U;
    }

    if (g_nco_enabled)
    {
        signed_sample = nco_run();
        dac_sample = (uint16_t)(signed_sample + 32768L);
        dac_sample >>= 4;
        sac_set_dac(dac_sample);
    }
}
