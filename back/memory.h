#ifndef MEMORY_included
#define MEMORY_included

#include <stdbool.h>
#include <stddef.h>

typedef struct segment_sizes {
    size_t ss_text;
    size_t ss_data;
    size_t ss_bss;
    size_t ss_free;
    size_t ss_stack;
} segment_sizes;

extern void init_memory_monitor  (void);

extern bool stack_has_overflowed (void);

extern void get_memory_use       (segment_sizes *);

extern void print_backtrace      (void);

#endif /* !MEMORY_included */
