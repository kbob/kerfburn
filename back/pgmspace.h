#include <stdint.h>

#define PROGMEM
#define PGM_P const char *
#define PSL(s) (s)

#define printf_P printf
#define strlen_P strlen
#define strncpy_P strncpy

static inline uintptr_t pgm_read_word(const void *addr)
{
    return (uintptr_t)addr;
}

static inline uint8_t pgm_read_byte(const void *addr)
{
    return *(uint8_t *)addr;
}
