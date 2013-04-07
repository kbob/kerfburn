// Override libc abort.  This version throbs the LED.

#include <stdlib.h>             // abort is declared here.

#include <stdbool.h>

#include <util/atomic.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include "config/pin-defs.h"
#include "pin-io.h"

static inline void light_LED(void)
{
    INIT_OUTPUT_PIN(LED, LED_ON);
}

static inline void extinguish_LED(void)
{
    INIT_OUTPUT_PIN(LED, LED_OFF);
}

static void throb(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        while (true) {
            int duty;
            for (duty = 0; duty < 30; duty++) {
                light_LED();
                _delay_ms(duty);
                extinguish_LED();
                _delay_ms(30 - duty);
            }
            for (duty = 30; duty > 0; --duty) {
                light_LED();
                _delay_ms(duty);
                extinguish_LED();
                _delay_ms(30 - duty);
            }
        }
    }
}

void abort(void)
{
    while (true)                // Compiler warning without this loop
        throb();
}
