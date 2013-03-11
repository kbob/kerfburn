#include "fw_stdio.h"

#include <stdio.h>

#include "serial.h"

static int my_putc(char c, FILE *stream)
{
    if (c == '\n')
        serial_tx_put_char('\r');
    serial_tx_put_char(c);
    return 0;
}

static FILE my_stdouterr = FDEV_SETUP_STREAM(my_putc, NULL, _FDEV_SETUP_WRITE);

void init_stdio(void)
{
    stdout = stderr = &my_stdouterr;
}
