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

void do_housekeeping(void)
{
#if 1
    printf("housekeeping\n");
    uint32_t t = 0;
    int i = 0;
    while (1) {
        while (!serial_rx_is_ready())
            continue;
        uint8_t e = serial_rx_errors();
        if (e) {
            printf("errors: %x\n", e);
            continue;

        }
        if (!t)
            t = millisecond_time() + 100;
        uint8_t c = serial_rx_peek_char(0);
        serial_rx_consume(1);
#if 1
        while (!serial_tx_is_available())
            continue;
        serial_tx_put_char(c);
#else
        extern uint8_t get_line_count(void);
        printf("%d %d %d: ", i, serial_rx_count(), get_line_count());
        if (32 < c && c < 0177)
            putchar(c);
        else
            printf("%03o", c);
        printf("\r\n");
#endif
        while (millisecond_time() < t)
            continue;
        t += 100;
        i++;
    }
#else
    printf("housekeeping\n");
    uint32_t t = millisecond_time();
    while (1) {
        while (millisecond_time() < t)
            continue;
        printf("%ld\n", t);
        t += 100;
    }
#endif
}

int main()
{
    initialize_devices();
    while (true)
        do_housekeeping();
}
