#ifndef SAC_H
#define SAC_H

#include <stdint.h>

void sac_init(void);
void sac_set_dac(uint16_t value);
void sac_set_zero(void);
void sac_set_full_scale(void);

#endif
