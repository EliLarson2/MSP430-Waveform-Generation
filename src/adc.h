#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
uint16_t adc_get_reading(void);
uint16_t adc_read_channel(uint8_t channel);

#endif
