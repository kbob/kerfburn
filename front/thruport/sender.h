#ifndef SENDER_included
#define SENDER_included

// Pass NULL to use standard input.
// Otherwise, pass a NULL-terminated list of file names.
extern int be_sender(const char **files);

extern void instantiate_sender_service(int sock);

extern void enable_sender(void);

#endif /* !SENDER_included */
