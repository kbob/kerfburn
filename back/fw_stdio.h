#ifndef FW_STDIO_included
#define FW_STDIO_included

#include <avr/pgmspace.h>

// PSL: program space string literal
#define PSL(s) ({ static const char x[] PROGMEM = (s); x; })

extern void init_stdio(void);

#endif /* !FW_STDIO_included */
