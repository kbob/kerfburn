#include "lasers.h"

#include <avr/interrupt.h>

#include "fault.h"

void init_lasers(void)
{
    // Initialize pins.
    INIT_OUTPUT_PIN(MAIN_LASER_PULSE,    MAIN_LASER_PULSE_OFF);
    INIT_OUTPUT_PIN(VISIBLE_LASER_PULSE, VISIBLE_LASER_PULSE_OFF);

    // Initialize timer.
    LASER_PULSE_TCCRA       = _BV(LASER_PULSE_WGM1);
    LASER_PULSE_TCCRB       = _BV(LASER_PULSE_WGM3) | _BV(LASER_PULSE_WGM2);
    MAIN_LASER_PULSE_OCR    = 0;
    VISIBLE_LASER_PULSE_OCR = 0;
    LASER_PULSE_TIFR       |= _BV(LASER_PULSE_TOV);
    LASER_PULSE_TIMSK       = _BV(LASER_PULSE_TOIE);

    // Set the laser watchdog
    LASER_WATCHDOG_OCR      = 0xFFFF;
    LASER_WATCHDOG_TIFR    |= _BV(LASER_WATCHDOG_OCF);
    LASER_WATCHDOG_TIMSK   |= _BV(LASER_WATCHDOG_OCIE);
}

ISR(LASER_WATCHDOG_TIMER_COMP_vect)
{
    trigger_fault(F_SI);
}
