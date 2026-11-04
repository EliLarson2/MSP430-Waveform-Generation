#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_write_string(const char *message);
void uart_send_adc_value(uint16_t value);
uint8_t uart_command_ready(void);
void uart_process_command(void);

#endif
