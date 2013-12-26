#ifndef LASERS_included
#define LASERS_included

#include "config/pin-defs.h"

#include "pin-io.h"

extern void init_lasers(void);

static inline void set_laser_pulse_interval(uint16_t interval)
{
    LASER_PULSE_ICR = interval;
}


// Main Laser

static inline void set_main_laser_continuous(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_ON);
    MAIN_LASER_PULSE_TCCRA &=
        ~(_BV(MAIN_LASER_PULSE_COM1) | _BV(MAIN_LASER_PULSE_COM0));
}

static inline void set_main_laser_start_on_match(void)
{
#if MAIN_LASER_PULSE_ON
    MAIN_LASER_PULSE_TCCRA |=
        _BV(MAIN_LASER_PULSE_COM1) | _BV(MAIN_LASER_PULSE_COM0);
#else
    const uint8_t com1 = _BV(MAIN_LASER_PULSE_COM1);
    const uint8_t com0 = _BV(MAIN_LASER_PULSE_COM0);
    MAIN_LASER_PULSE_TCCRA = MAIN_LASER_PULSE_TCCRA & ~(com1 | com0) | com1;
#endif
}

static inline void set_main_laser_stop_on_match(void)
{
#if MAIN_LASER_PULSE_ON
    MAIN_LASER_PULSE_TCCRA = (MAIN_LASER_PULSE_TCCRA |
                              _BV(MAIN_LASER_PULSE_COM1)) &
        ~_BV(MAIN_LASER_PULSE_COM0);
#else
    MAIN_LASER_PULSE_TCCRA |=
        _BV(MAIN_LASER_PULSE_COM1) | _BV(MAIN_LASER_PULSE_COM0);
#endif
}

static inline void set_main_laser_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    MAIN_LASER_PULSE_TCCRA &=
        ~(_BV(MAIN_LASER_PULSE_COM1) | _BV(MAIN_LASER_PULSE_COM0));
}

static inline void set_main_laser_pulse_duration(uint16_t duration)
{
    MAIN_LASER_PULSE_OCR = duration;
}


// Visible Laser

static inline void set_visible_laser_continuous(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_ON);
    VISIBLE_LASER_PULSE_TCCRA &= ~_BV(VISIBLE_LASER_PULSE_COM1);
}

static inline void set_visible_laser_pulsed(void)
{
    VISIBLE_LASER_PULSE_TCCRA |= _BV(VISIBLE_LASER_PULSE_COM1);
}

static inline void set_visible_laser_off(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
    VISIBLE_LASER_PULSE_TCCRA &= ~_BV(VISIBLE_LASER_PULSE_COM1);
}

static inline void set_visible_laser_pulse_duration(uint16_t duration)
{
    VISIBLE_LASER_PULSE_OCR = duration;
}

static inline void stop_pulse_timer(void)
{
    LASER_PULSE_TCCRB &= ~_BV(LASER_PULSE_CS0);
}

#endif /* !LASERS_included */
