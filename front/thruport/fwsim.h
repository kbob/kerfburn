#ifndef FWSIM_included
#define FWSIM_included

#include <stddef.h>
#include <unistd.h>

#define TTY_BUFSIZ 256

extern int     init_fwsim     (const char *command);
extern int     open_fwsim     (void);
extern void    close_fwsim    (void);

extern int     fwsim_transmit (const char *buf, size_t count);
extern ssize_t fwsim_receive  (      char *buf, size_t max);

#endif /* !FWSIM_included */
