#include "fw_stdio.h"

#include <stdio.h>

#include "serial.h"

static int my_putc(char c, FILE *stream)
{
    if (c == '\n') {
        while (!serial_tx_is_available())
            continue;
        serial_tx_put_char('\r');
    }
    while (!serial_tx_is_available())
        continue;
    serial_tx_put_char(c);
    if (c == '\n')
        while (!serial_tx_is_idle())
            continue;
    return 0;
}

static FILE my_stdouterr = FDEV_SETUP_STREAM(my_putc, NULL, _FDEV_SETUP_WRITE);

void init_stdio(void)
{
    stdout = stderr = &my_stdouterr;
}
