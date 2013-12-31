#ifndef FW_STDIO_included
#define FW_STDIO_included

#include <avr/pgmspace.h>

// PSL: program space string literal
// #define PSL(s) ({ static const char x[] PROGMEM = (s); x; })
#define PSL(s) (PSTR((s)))

extern void init_stdio(void);

#ifndef PRINTF_COMPAT
  #define printf    is deprecated, please use    printf_P
  #define fprintf   is deprecated, please use   fprintf_P
  #define sprintf   is deprecated, please use   sprintf_P
  #define snprintf  is deprecated, please use  snprintf_P
// AVR-libc is missing vprintf_P.
  #define vfprintf  is deprecated, please use  vfprintf_P
  #define vsprintf  is deprecated, please use  vsprintf_P
  #define vsnprintf is deprecated, please use vsnprintf_P
#endif

#endif /* !FW_STDIO_included */
