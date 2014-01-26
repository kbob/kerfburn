#include "trace.h"

#ifndef FW_NDEBUG

#include <stdio.h>

#include <avr/pgmspace.h>

struct trace_private trace_private;

void print_trace(void)
{
    size_t count = trace_private.tp_pos;
    printf_P(PSTR("\nTRACE\n"));
    for (size_t i = 0; i < count; i++) {
        const trace_entry *ep = &trace_private.tp_buffer[i];
        printf_P(PSTR("%s:%d: "), ep->te_func, ep->te_line);
        uint8_t c = ep->te_code;
        printf_P(PSTR("%2x "), c);
        if (c < ' ' || c >= '\177')
            c = '.';
        putchar(c);
        putchar('\n');
    }
    putchar('\n');
}

#endif /* !FW_NDEBUG */
