#include <stdbool.h>
#include <stdio.h>

#include <util/delay.h>

#include "fw_assert.h"
#include "fw_stdio.h"
#include "serial.h"
#include "timer.h"

static void initialize_devices(void)
{
    init_timer();
    init_serial();
    init_stdio();
    sei();
}

void do_background_task(void)
{
    serial_rx_start();
    uint32_t t = 0;
    while (1) {
        while (!serial_rx_has_chars())
            continue;
        uint8_t e = serial_rx_errors();
        if (e) {
            printf("\nerrors: %#x\n", e);
            continue;
        }
        if (!t)
            t = millisecond_time() + 100;
        uint8_t c = serial_rx_peek_char(0);
        serial_rx_consume(1);
        while (!serial_tx_is_available())
            continue;
        serial_tx_put_char(c);
        if (c == '\n') {
            while (millisecond_time() < t)
                continue;
            t += 25;
        }
    }
}

int main()
{
    initialize_devices();
    while (true)
        do_background_task();
}
