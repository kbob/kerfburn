#ifndef CLIENT_included
#define CLIENT_included

#include <stdbool.h>

typedef enum client_type {
    CT_CONTROLLER = 'C',
    CT_SENDER     = 'S',
    CT_RECEIVER   = 'R',
    CT_SUSPENDER  = 'Z',        // mnemonic: ^Z suspends a job.
} client_type;

extern bool daemon_is_running(const char *device);

// Returns socket descriptor or -1.
extern int  connect_to_daemon(client_type);

#endif /* !CLIENT_included */
