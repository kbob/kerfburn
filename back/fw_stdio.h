#ifndef FW_STDIO_included
#define FW_STDIO_included

#include <avr/pgmspace.h>

extern void init_stdio(void);

#ifndef PRINTF_COMPAT
  #define printf    printf_is_deprecated, please use    printf_P
  #define fprintf   fprintf_is_deprecated, please use   fprintf_P
  #define sprintf   sprintf_is_deprecated, please use   sprintf_P
  #define snprintf  snprintf_is_deprecated, please use  snprintf_P
// AVR-libc is missing vprintf_P.
  #define vfprintf  vfprintf_is_deprecated, please use  vfprintf_P
  #define vsprintf  vsprintf_is_deprecated, please use  vsprintf_P
  #define vsnprintf vsnprintf_is_deprecated, please use vsnprintf_P
#endif

#endif /* !FW_STDIO_included */
