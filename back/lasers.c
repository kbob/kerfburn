#include "lasers.h"

#include <avr/interrupt.h>

#include "fw_assert.h"

void init_lasers(void)
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
    LASER_PULSE_TIFR       |= _BV(LASER_PULSE_TOV);
}
