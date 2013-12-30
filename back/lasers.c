#include "lasers.h"

#include <avr/interrupt.h>

#include "fault.h"

void init_lasers(void)
{
    // Initialize pins.
    INIT_OUTPUT_PIN(MAIN_LASER_PULSE,    MAIN_LASER_PULSE_OFF);
    INIT_OUTPUT_PIN(VISIBLE_LASER_PULSE, VISIBLE_LASER_PULSE_OFF);
}

ISR(LASER_WATCHDOG_TIMER_COMP_vect)
{
    trigger_fault(F_SI);
}
