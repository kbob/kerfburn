#include <stdbool.h>

#include "parser.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"

// Test these.
// DONE millisecond time updates.
// DONE softint triggered from background
//      softint triggered from softint
//      softint triggered from softint

volatile bool toggler_enqueued = false;
uint32_t inc = 2000;
uint32_t next = 0;

static void initialize_devices(void)
{
    init_variables();
    init_timer();
    init_serial();
    // XXX more devices coming...
    sei();
}

static void toggle_LED(void)
{
    // LED is pin D13, aka PB7.
    DDRB |= _BV(DDB7);
    if (bit_is_set(PORTB, PB7))
        PORTB &= ~_BV(PB7);
    else
        PORTB |= _BV(PB7);
}

void toggler_done(void)
{
    toggle_LED();
    toggler_enqueued = false;
}

void do_housekeeping(void)
{
#if 0
    // If the status packet flag is set, generate status packet
    // If we've received a full command packet, enqueue the step generator.
#endif

#if 0
    // Test millisecond_time().

    uint32_t next = millisecond_time() + 1000;
    while (true) {
        uint32_t now = millisecond_time();
        if ((int)now - (int)next >= 0) {
            toggle_LED();
            next += 1000;
        }
    }
#else
    // Test triggering soft interrupt from base level.

    if (!toggler_enqueued) {
        toggler_enqueued = true;
        next += inc;
        if ((inc >>= 1) < 1) {
            inc = 2000;
        }
        for (int i = 0; i < 300; i++) {
            while (!serial_tx_is_available())
                continue;
            serial_tx_put_char('/' + i % 11);
        }
       while (!serial_tx_is_available())
            continue;
        serial_tx_put_char('\r');
        while (!serial_tx_is_available())
            continue;
        serial_tx_put_char('\n');
        static timeout toggler;
        toggler.to_func = toggler_done;
        enqueue_timeout(&toggler, next);
    }
#endif
    parse_line();
}

int main()
{
    initialize_devices();
    while (true)
        do_housekeeping();
}
