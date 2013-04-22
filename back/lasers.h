#ifndef LASERS_included

#include "config/pin-defs.h"

#include "pin-io.h"

static inline void init_lasers(void)
{
    INIT_OUTPUT_PIN(MAIN_LASER_PULSE,    MAIN_LASER_PULSE_OFF);
    INIT_OUTPUT_PIN(VISIBLE_LASER_PULSE, VISIBLE_LASER_PULSE_OFF);
}

static inline void fire_main_laser(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_ON);
}

static inline void stop_main_laser(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
}

static inline void fire_visible_laser(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_ON);
}

static inline void stop_visible_laser(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
}

#endif /* !LASERS_included */
