#ifndef RECEIVER_CLIENT_included
#define RECEIVER_CLIENT_included

// Pass NULL to send standard input.
// Otherwise, pass a NULL-terminated list of file names.
// (E.g., the tail of argv.)
//
// Returns process exit status.
extern int be_receiver(const char **files);

#endif /* !RECEIVER_CLIENT_included */
