#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint8_t g_nco_enabled;
extern volatile uint8_t g_led_enabled;

void timer_init(void);

#endif
