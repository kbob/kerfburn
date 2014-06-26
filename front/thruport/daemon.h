#ifndef DAEMON_included
#define DAEMON_included

#include <stdbool.h>

// Start daemon in background; current process need not exit.
extern int spawn_daemon(void);

// Start daemon, possibly in foreground.
extern int start_daemon(bool debug, const char *fwsim);

extern int suspend_daemon(void);
extern int resume_daemon(void);

#endif /* !DAEMON_included */
