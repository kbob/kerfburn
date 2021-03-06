#ifndef SERIAL_included
#define SERIAL_included

#include <stddef.h>
#include <unistd.h>

#define TTY_BUFSIZ 256

extern int     init_serial     (void);
extern int     open_serial     (void);
extern void    close_serial    (void);

extern int     serial_transmit (const char *buf, size_t count);
extern ssize_t serial_receive  (      char *buf, size_t max);

#endif /* !SERIAL_included */
