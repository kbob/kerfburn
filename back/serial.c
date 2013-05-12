#include "serial.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RX_BUF_SIZE 256
static char rx_buf[RX_BUF_SIZE];
size_t rx_pos, rx_end;

static void load_buf(void)
{
    if (!fgets(rx_buf, sizeof rx_buf, stdin)) {
        if (feof(stdin))
            exit(EXIT_SUCCESS);
        perror("stdin");
        exit(EXIT_FAILURE);
    }
    rx_pos = 0; rx_end = strlen(rx_buf);
}

void init_serial(void)
{}

uint8_t serial_rx_errors(void)
{
    return 0;
}

bool serial_rx_has_chars(void)
{
    if (rx_pos == rx_end)
        load_buf();
    return true;
}

bool serial_rx_has_lines(void)
{
    if (rx_pos == rx_end)
        load_buf();
    return true;
}

uint8_t serial_rx_peek_char(uint8_t pos)
{
    assert(rx_pos + pos < rx_end);
    return rx_buf[rx_pos + pos];
}

void serial_rx_consume(uint8_t count)
{
    assert(rx_pos + count <= rx_end);
    rx_pos += count;
}
