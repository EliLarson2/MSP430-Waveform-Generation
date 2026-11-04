#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "adc.h"
#include "nco.h"
#include "pwm.h"
#include "sac.h"
#include "timer.h"

#define UART_COMMAND_LENGTH 2U
#define UART_BUFFER_SIZE    6U

static volatile char command_buffer[UART_BUFFER_SIZE];
static volatile uint8_t command_index = 0U;
static volatile uint8_t command_ready = 0U;

void uart_init(void)
{
    /* Hold eUSCI_A1 in reset and select SMCLK. */
    UCA1CTLW0 = UCSWRST | UCSSEL__SMCLK;

    /* 57,600 baud with 24 MHz SMCLK, from the original lab setup. */
    UCA1BRW = 26U;
    UCA1MCTLW = (0xD6U << 8) | UCOS16;

    /* P4.2 = TXD, P4.3 = RXD. */
    P4SEL1 &= ~(BIT2 | BIT3);
    P4SEL0 |=  (BIT2 | BIT3);

    UCA1CTLW0 &= ~UCSWRST;
    UCA1IE |= UCRXIE;
}

void uart_write_string(const char *message)
{
    while (*message != '\0')
    {
        while (!(UCA1IFG & UCTXIFG))
        {
        }

        UCA1TXBUF = (uint8_t)*message;
        message++;
    }
}

void uart_send_adc_value(uint16_t value)
{
    char buffer[8];
    (void)snprintf(buffer, sizeof(buffer), "%u\r\n", value);
    uart_write_string(buffer);
}

uint8_t uart_command_ready(void)
{
    return command_ready;
}

static void execute_command(const char *command)
{
    if (strcmp(command, "R0") == 0)
    {
        P1OUT &= ~BIT0;
        g_led_enabled = 0U;
    }
    else if (strcmp(command, "R1") == 0)
    {
        P1OUT |= BIT0;
        g_led_enabled = 0U;
    }
    else if (strcmp(command, "G0") == 0)
    {
        P6OUT &= ~BIT6;
    }
    else if (strcmp(command, "G1") == 0)
    {
        P6OUT |= BIT6;
    }
    else if (command[0] == 'A' && command[1] >= '1' && command[1] <= '5')
    {
        uint8_t channel = (uint8_t)(command[1] - '0');
        uart_send_adc_value(adc_read_channel(channel));
    }
    else if (strcmp(command, "M0") == 0)
    {
        motor_stop();
    }
    else if (strcmp(command, "M1") == 0)
    {
        motor_clockwise();
    }
    else if (strcmp(command, "M2") == 0)
    {
        motor_counterclockwise();
    }
    else if (strcmp(command, "D0") == 0)
    {
        g_nco_enabled = 0U;
        nco_stop();
        sac_set_zero();
    }
    else if (strcmp(command, "D1") == 0)
    {
        nco_set_frequency(40.0f);
        g_nco_enabled = 1U;
    }
    else if (strcmp(command, "D2") == 0)
    {
        nco_set_frequency(80.0f);
        g_nco_enabled = 1U;
    }
    else if (strcmp(command, "D3") == 0)
    {
        nco_set_frequency(100.0f);
        g_nco_enabled = 1U;
    }
    else if (strcmp(command, "D4") == 0)
    {
        g_nco_enabled = 0U;
        nco_stop();
        sac_set_full_scale();
    }
}

void uart_process_command(void)
{
    char local_command[UART_BUFFER_SIZE];
    uint8_t i;

    if (!command_ready)
    {
        return;
    }

    __disable_interrupt();
    for (i = 0U; i < UART_BUFFER_SIZE; i++)
    {
        local_command[i] = command_buffer[i];
    }
    command_ready = 0U;
    __enable_interrupt();

    execute_command(local_command);
}

#pragma vector=EUSCI_A1_VECTOR
__interrupt void UART_ISR(void)
{
    uint8_t received_char = UCA1RXBUF;

    if (received_char == '\r' || received_char == '\n')
    {
        command_index = 0U;
        return;
    }

    if (command_ready)
    {
        return;
    }

    if (command_index < UART_COMMAND_LENGTH)
    {
        command_buffer[command_index++] = (char)received_char;
    }

    if (command_index >= UART_COMMAND_LENGTH)
    {
        command_buffer[command_index] = '\0';
        command_ready = 1U;
        command_index = 0U;
    }
}
