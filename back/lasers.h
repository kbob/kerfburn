#ifndef LASERS_included

#include "config/pin-defs.h"

#include "pin-io.h"

static inline void init_lasers(void)
{
    // Initialize pins.
    INIT_OUTPUT_PIN(MAIN_LASER_PULSE,    MAIN_LASER_PULSE_OFF);
    INIT_OUTPUT_PIN(VISIBLE_LASER_PULSE, VISIBLE_LASER_PULSE_OFF);

    // Initialize timer.
    LASER_PULSE_TCCRA       = _BV(LASER_PULSE_WGM1);
    LASER_PULSE_TCCRB       = _BV(LASER_PULSE_WGM3) | _BV(LASER_PULSE_WGM2);
    MAIN_LASER_PULSE_OCR    = 1;
    VISIBLE_LASER_PULSE_OCR = 1;
    LASER_PULSE_TIMSK       = _BV(LASER_PULSE_TOIE);
    LASER_PULSE_TIFR        = _BV(LASER_PULSE_TOV);
}

static inline void set_laser_pulse_interval(uint16_t interval)
{
    LASER_PULSE_ICR = interval;
}


// Main Laser

static inline void set_main_laser_continuous(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_ON);
    MAIN_LASER_PULSE_TCCRA &= ~_BV(MAIN_LASER_PULSE_COM1);
}

static inline void set_main_laser_pulsed(void)
{
    MAIN_LASER_PULSE_TCCRA |= _BV(MAIN_LASER_PULSE_COM1);
}

static inline void set_main_laser_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    MAIN_LASER_PULSE_TCCRA &= ~_BV(MAIN_LASER_PULSE_COM1);
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
