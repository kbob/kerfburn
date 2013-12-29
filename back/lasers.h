#ifndef LASERS_included
#define LASERS_included

#include "config/pin-defs.h"

#include "pin-io.h"

extern void init_lasers(void);

static inline void stop_pulse_timer_NONATOMIC(void)
{
    // Stop clock; WGM[3:2] = normal mode
    LASER_PULSE_TCCRB = 0;

    if (REG_BIT_IS(MAIN_LASER_PULSE_PIN, MAIN_LASER_PULSE_ON)) {
#if MAIN_LASER_PULSE_ON
        // COM[1:0] = clear on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = _BV(MAIN_LASER_PULSE_COM1);
#else
        // COM[1:0] = set on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                             _BV(MAIN_LASER_PULSE_COM0));
#endif
        // Strobe Force Output Compare.
        LASER_PULSE_TCCRC = _BV(MAIN_LASER_PULSE_FOC);
    }
    // Clear port output.
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);

    if (REG_BIT_IS(VISIBLE_LASER_PULSE_PIN, VISIBLE_LASER_PULSE_ON)) {
#if VISIBLE_LASER_PULSE_ON
        // COM[1:0] = clear on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = _BV(VISIBLE_LASER_PULSE_COM1);
#else
        // COM[1:0] = set on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                             _BV(VISIBLE_LASER_PULSE_COM0));
#endif
        // Strobe Force Output Compare.
        LASER_PULSE_TCCRC = _BV(VISIBLE_LASER_PULSE_FOC);
    }
    // Clear port output.
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);

    // Clear all.
    LASER_PULSE_TCCRA = 0;
    LASER_PULSE_TCNT  = 0;
    LASER_PULSE_TIMSK = 0;
    LASER_PULSE_TIFR  = 0;
}

static inline void set_laser_pulse_interval(uint16_t interval)
{
    LASER_PULSE_ICR = interval;
}

static inline void set_lasers_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
    MAIN_LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}


// Main Laser

static inline void set_main_laser_on(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_ON);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void set_main_laser_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void set_main_laser_start_on_timer(void)
{
#if MAIN_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(MAIN_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#endif
}

static inline void set_main_laser_stop_on_timer(void)
{
#if MAIN_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(MAIN_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#endif
}


// Visible Laser

static inline void set_visible_laser_on(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_ON);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void set_visible_laser_off(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void set_visible_laser_start_on_timer(void)
{
#if VISIBLE_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(VISIBLE_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#endif
}

static inline void set_visible_laser_stop_on_timer(void)
{
#if VISIBLE_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(VISIBLE_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#endif
}


#endif /* !LASERS_included */
