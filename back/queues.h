#ifndef QUEUES_included
#define QUEUES_included

#include <stdint.h>

typedef uint16_t atom;
typedef uint8_t queue;

// Queues for X, Y, Z motors and main and visible lasers
extern queue Xq, Yq, Zq, MLq, VLq;

extern void  init_queues             (void);

extern void  start_queues            (void);
extern void  await_queues            (void);
extern void  stop_queues_immediately (void);

extern void  enqueue_atom            (atom, queue);

#endif /* !QUEUES_included */
