#include "fw_stdio.h"

#include <stdio.h>

#include <avr/io.h>
#include <util/atomic.h>

#include "fw_assert.h"
#include "serial.h"

static int putc_stout(char c, FILE *stream)
{
    if (c == '\n')
        putc_stout('\r', stream);
    while (!serial_tx_is_available())
        continue;
    bool ok = serial_tx_put_char(c);
    ok = ok;
    fw_assert(ok);
    if (c == '\n')
        while (!serial_tx_is_idle())
            continue;
    return 0;
}

static int putc_sterr(char c, FILE *stream)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (c == '\n') {
            loop_until_bit_is_set(UCSR0A, UDRE0);
            UDR0 = '\r';
        }
        loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = c;
    }
    return 0;
}

static FILE fw_stout = FDEV_SETUP_STREAM(putc_stout, NULL, _FDEV_SETUP_WRITE);
static FILE fw_sterr = FDEV_SETUP_STREAM(putc_sterr, NULL, _FDEV_SETUP_WRITE);

void init_stdio(void)
{
    stdout = &fw_stout;
    stderr = &fw_sterr;
}
