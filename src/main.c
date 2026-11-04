#include <msp430.h>

#include "adc.h"
#include "nco.h"
#include "pwm.h"
#include "sac.h"
#include "timer.h"
#include "uart.h"

static void gpio_init(void)
{
    /* Red LED: P1.0 */
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    /* Green LED: P6.6 */
    P6DIR |= BIT6;
    P6OUT &= ~BIT6;

    /* Release GPIO from the default high-impedance state. */
    PM5CTL0 &= ~LOCKLPM5;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    /* The original laboratory configuration used a 24 MHz SMCLK. */
    gpio_init();
    uart_init();
    adc_init();
    pwm_init();
    sac_init();
    nco_init();
    timer_init();

    __enable_interrupt();

    while (1)
    {
        if (uart_command_ready())
        {
            uart_process_command();
        }
    }
}
