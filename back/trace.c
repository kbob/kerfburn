#include "trace.h"

#ifndef FW_NDEBUG

#include <stdio.h>

#include <avr/pgmspace.h>

#include "atoms.h"

struct trace_private trace_private;

void print_trace(void)
{
    size_t count = trace_private.tp_pos;
    printf_P(PSTR("\nTRACE\n"));
    for (size_t i = 0; i < count; i++) {
        const trace_entry *ep = &trace_private.tp_buffer[i];
        printf_P(PSTR("%s:%u: "), ep->te_func, ep->te_line);
        uint16_t code = ep->te_code;
        uint8_t c = ep->te_code;
        printf_P(PSTR("%4"PRIx16" "), code);
        if (c < ' ' || c >= '\177')
            c = '.';
        putchar(c);
        if (is_atom(code))
            print_atom(" ", code);
        putchar('\n');
    }
    putchar('\n');
}

#endif /* !FW_NDEBUG */
