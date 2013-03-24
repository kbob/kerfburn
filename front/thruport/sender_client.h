#ifndef SENDER_CLIENT_included
#define SENDER_CLIENT_included

// Pass NULL to send standard input.
// Otherwise, pass a NULL-terminated list of file names.
// (E.g., the tail of argv.)
//
// Returns process exit status.
extern int be_sender(const char **files);

#endif /* !SENDER_CLIENT_included */
