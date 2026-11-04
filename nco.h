#ifndef NCO_H
#define NCO_H

#include <stdint.h>

void nco_init(void);
void nco_set_frequency(float frequency_hz);
void nco_stop(void);
int16_t nco_run(void);

#endif
