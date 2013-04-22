#ifndef QUEUES_included
#define QUEUES_included

#include <stdbool.h>
#include <stdint.h>

#include "atoms.h"

#include "fw_assert.h"


// Interface

typedef struct queue_private queue;

// Queues for X, Y, Z motors and laser pulses.
extern queue Xq, Yq, Zq, Pq;

static inline void init_queues             (void);

static inline bool queue_is_running        (const queue *);
static inline bool queue_is_empty          (const queue *);
static inline bool queue_is_full           (const queue *);

extern        void start_queues            (void);
extern        void await_queues_empty      (void);
extern        void stop_queues_immediately (void);

static inline void enqueue_atom_X          (atom);
static inline atom dequeue_atom_X          (void);

static inline void enqueue_atom_Y          (atom);
static inline atom dequeue_atom_Y          (void);

static inline void enqueue_atom_Z          (atom);
static inline atom dequeue_atom_Z          (void);

static inline void enqueue_atom_P          (atom);
static inline atom dequeue_atom_P          (void);


// Implementation

struct queue_private {
    volatile uint8_t q_head;    // volatile so baselevel can read without lock
    uint8_t          q_tail;
    bool             q_is_running;
};

typedef uint16_t queue_buf[256 / 2] __attribute__((aligned(256)));

extern queue_buf Xq_buf, Yq_buf, Zq_buf, Pq_buf;

static inline void init_queues(void)
{
    // Already zeroed.
}

static inline bool queue_is_running(const queue *q)
{
    return q->q_is_running;
}

static inline bool queue_is_empty(const queue *q)
{
    return q->q_head == q->q_tail;
}

static inline bool queue_is_full(const queue *q)
{
    return q->q_head == (uint8_t)(q->q_tail + 2);
}

#define DEFINE_ENQUEUE_DEQUEUE(Q)               \
                                                \
    static inline void enqueue_atom_##Q(atom a) \
    {                                           \
        union {                                 \
            uint8_t b[2];                       \
            int    *p;                          \
        } u;                                    \
                                                \
        fw_assert(!queue_is_full(&Q##q));       \
        u.b[0] = Q##q.q_tail;                   \
        u.b[1] = (uintptr_t)Q##q_buf >> 8;      \
        *u.p++ = a;                             \
        Q##q.q_tail = u.b[0];                   \
    }                                           \
                                                \
    static inline atom dequeue_atom_##Q(void)   \
    {                                           \
        union {                                 \
            uint8_t b[2];                       \
            int    *p;                          \
        } u;                                    \
                                                \
        fw_assert(!queue_is_empty(&Q##q));      \
        u.b[0] = Q##q.q_head;                   \
        u.b[1] = (uintptr_t)Q##q_buf >> 8;      \
        atom a = *u.p++;                        \
        Q##q.q_head = u.b[0];                   \
        return a;                               \
    }

DEFINE_ENQUEUE_DEQUEUE(X);
DEFINE_ENQUEUE_DEQUEUE(Y);
DEFINE_ENQUEUE_DEQUEUE(Z);
DEFINE_ENQUEUE_DEQUEUE(P);

#undef DEFINE_ENQUEUE_DEQUEUE

#endif /* !QUEUES_included */
