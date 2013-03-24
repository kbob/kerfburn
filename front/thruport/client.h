#ifndef CLIENT_included
#define CLIENT_included

typedef enum client_type {
    CT_CONTROLLER = 'C',
    CT_SENDER     = 'S',
    CT_RECEIVER   = 'R',
    CT_SUSPENDER  = 'Z',        // mnemonic: ^Z suspends a job.
} client_type;

// XXX factor this out.  Have two functions, connect_to_daemon
//     and connect_or_start_daemon.
// Returns socket descriptor or -1 w/ errno set.
extern int  connect_to_daemon(client_type);

#endif /* !CLIENT_included */
