#ifndef CLIENT_included
#define CLIENT_included

#include <stdbool.h>

extern bool daemon_is_running(const char *device);

// Returns socket descriptor or -1.
extern int  connect_to_daemon(const char *device);

#endif /* !CLIENT_included */
