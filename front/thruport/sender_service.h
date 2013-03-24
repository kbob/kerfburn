#ifndef SENDER_SERVICE_included
#define SENDER_SERVICE_included

extern void  instantiate_sender_service (int sock);
extern void  disconnect_sender          (void);

extern int   await_sender_socket        (void);
extern void  report_sender_error        (int priority, const char *msg);

#endif /* !SENDER_SERVICE_included */
