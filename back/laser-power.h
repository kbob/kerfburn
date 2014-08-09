#ifndef LASER_POWER_included
#define LASER_POWER_included

#include <stdint.h>

#define MIN_LASER_POWER 0
#define MAX_LASER_POWER 4095

extern void init_laser_power(void);

extern void set_laser_power(uint16_t level);

#endif /* !LASER_POWER_included */
