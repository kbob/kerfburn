#ifndef SUSPENDER_CLIENT_included
#define SUSPENDER_CLIENT_included

// Pass NULL to send standard input.
// Otherwise, pass a NULL-terminated list of file names.
// (E.g., the tail of argv.)
//
// Returns process exit status.
extern int be_suspender(const char **argv);

#endif /* !SUSPENDER_CLIENT_included */
