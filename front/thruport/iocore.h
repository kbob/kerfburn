#ifndef IOCORE_included
#define IOCORE_included

// This is a veneer over select(2) or poll(2) (or epoll or kqueue or...).
// It allows event-driven I/O for a single-threaded program.
// It is used by daemon and by sender, and maybe others.

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
