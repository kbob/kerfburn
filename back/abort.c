// Override libc abort.  This version throbs the LED.

#include <stdlib.h>             // abort is declared here.

#include <stdbool.h>

#include <util/atomic.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

static inline void light_LED(void)
{
    DDRB  |= _BV(DDB7);
    PORTB |= _BV(PB7);
}

static inline void extinguish_LED(void)
{
    DDRB  |= _BV(DDB7);
    PORTB &= ~_BV(PB7);
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
