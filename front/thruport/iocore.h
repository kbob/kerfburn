#ifndef IOCORE_included
#define IOCORE_included

typedef enum io_event_set {
    IE_READ   = 1 << 0,
    IE_WRITE  = 1 << 1,
    IE_EXCEPT = 1 << 2
} io_event_set;

typedef void io_event_handler(int fd, io_event_set events, void *closure);

extern void run_iocore            (void) __attribute__((noreturn));

extern int io_register_descriptor (int               fd,
                                   io_event_set      events,
                                   io_event_handler *handler,
                                   void             *closure);

extern int io_enable_descriptor   (int               fd,
                                   io_event_set      events);

extern int io_delist_descriptor   (int               fd);

#endif /* !IOCORE_included */
