#include "trace.h"

#include <stdio.h>

struct trace_private trace_private;

void print_trace(void)
{
    printf("\nTRACE\n");
    size_t count = trace_private.tp_pos;
    for (size_t i = 0; i < count; i++) {
        const trace_entry *ep = &trace_private.tp_buffer[i];
        printf("%s:%d: ", ep->te_func, ep->te_line);
        uint8_t c = ep->te_code;
        printf("%2x ", c);
        if (c < ' ' || c >= '\177')
            c = '.';
        putchar(c);
        putchar('\n');
    }        
    putchar('\n');
}
