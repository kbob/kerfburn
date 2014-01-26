#include "memory.h"

#include <stdint.h>
#include <stdio.h>

#include <avr/pgmspace.h>

extern const uint8_t _etext;
extern const uint8_t __data_start;
extern const uint8_t _edata;
extern       uint8_t _end;
extern const uint8_t __stack;


static void *return_arg(void *p)
{
    return p;
}

static void *a_ptr(void)
{
    auto int x;
    return return_arg(&x);
}

static size_t measure_free(void)
{
    uint8_t x = 0;
    const uint8_t *p;
    for (p = &_end; ; p++)
        if (*p != (x += 37))
            break;
    return p - &_end;
}

void init_memory_monitor(void)
{
    uint8_t x = 0;
    uint8_t *stack_top = a_ptr();
    extern uint8_t _end;
    for (uint8_t *p = &_end; p < stack_top; p++)
        *p = x += 37;
}

bool stack_has_overflowed(void)
{
    return measure_free() >= 10;
}

void get_memory_use(segment_sizes *ssp)
{
    size_t free = measure_free();
    ssp->ss_text  = (size_t)&_etext;
    ssp->ss_data  = (size_t)&_edata - (size_t)&__data_start;;
    ssp->ss_bss   = (size_t)&_end - (size_t)&_edata;
    ssp->ss_free  = free;
    ssp->ss_stack = (size_t)&__stack + 1 - (size_t)&_end - free;
}

void print_backtrace(void)
{
    auto uint8_t x;
    printf_P(PSTR("STACK\n"));
    for (const uint8_t *p = &x; p < &__stack; p++)
        if (*(uint8_t **)p < &_etext)
            printf_P(PSTR("%6p\n"), *(uint8_t **)p);
    putchar('\n');
}
