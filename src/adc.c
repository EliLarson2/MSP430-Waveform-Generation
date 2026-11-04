#include <msp430.h>
#include "adc.h"

void adc_init(void)
{
    /*
     * ADCSHT = 1: 8 ADCCLK sample-and-hold period
     * ADCON = 1: ADC on
     * ADCENC = 0 while configuring
     */
    ADCCTL0 = (1U << 8) | (1U << 4);

    /*
     * ADCSHP = 1: sampling timer source
     * ADCDIV = 7: divide SMCLK by 8
     * ADCSSEL = 3: SMCLK source
     * ADCCONSEQ = 0: single-channel, single-conversion
     */
    ADCCTL1 = (1U << 9) | (7U << 5) | (3U << 3);

    /* ADCRES = 2: 12-bit conversion. */
    ADCCTL2 = (2U << 4);

    /* AVCC/AVSS reference, default channel A2. */
    ADCMCTL0 = (0U << 4) | (2U << 0);

    /* Enable conversion. */
    ADCCTL0 |= ADCENC;
}

uint16_t adc_get_reading(void)
{
    ADCCTL0 |= ADCSC;

    /* Wait while conversion is active. */
    while (ADCCTL1 & ADCBUSY)
    {
    }

    return ADCMEM0;
}

uint16_t adc_read_channel(uint8_t channel)
{
    if (channel < 1U || channel > 5U)
    {
        return 0U;
    }

    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH_15;
    ADCMCTL0 |= channel;
    ADCCTL0 |= ADCENC;

    return adc_get_reading();
}
