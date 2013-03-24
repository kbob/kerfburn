#ifndef SENDER_SERVICE_included
#define SENDER_SERVICE_included

#include <stdio.h>

extern void  instantiate_sender_service (int sock);
extern void  disconnect_sender          (void);

extern int   await_sender_socket        (void);
//extern FILE *get_sender_out_stream      (void);
extern void  report_sender_error        (int priority, const char *msg);

#endif /* !SENDER_SERVICE_included */
