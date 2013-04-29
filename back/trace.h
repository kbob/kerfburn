#ifndef TRACE_included
#define TRACE_included

#include <stddef.h>
#include <stdint.h>

#include <util/atomic.h>


// Interface

#define TRACE(x) (trace(__func__, __LINE__, (x)))

static inline void reset_trace (void);
extern        void print_trace (void);


// Implementation

#define TRACE_SIZE 50

typedef struct trace_entry {
    const char *te_func;
    uint16_t    te_line;
    uint8_t     te_code;
} trace_entry;

extern struct trace_private {
    size_t      tp_pos;
    trace_entry tp_buffer[TRACE_SIZE];
} trace_private;

static inline void trace(const char *func, uint16_t line, uint8_t code)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (trace_private.tp_pos < TRACE_SIZE) {
            trace_entry *ep = &trace_private.tp_buffer[trace_private.tp_pos++];
            ep->te_func = func;
            ep->te_line = line;
            ep->te_code = code;
        }
    }
}

static inline void reset_trace(void)
{
    trace_private.tp_pos = 0;
}

#if 0
static inline void trace(uint8_t x)
{
    if (trace_private.tp_pos < TRACE_SIZE)
        trace_private.tp_buffer[trace_private.tp_pos++] = x;
}
#endif


#endif /* !TRACE_included */
