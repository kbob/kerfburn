#ifndef QUEUES_included
#define QUEUES_included

#include <stdbool.h>
#include <stdint.h>

#include "atoms.h"


// Interface

typedef struct queue_private queue;

// Queues for X, Y, Z motors and laser pulses.
extern queue Xq, Yq, Zq, Pq;

extern void     init_queues                (void);

extern bool     queue_is_empty             (const queue *);
extern bool     queue_is_full              (const queue *);
extern bool     queue_is_empty_NONATOMIC   (const queue *);
extern bool     queue_is_full_NONATOMIC    (const queue *);
extern bool     any_queue_is_full          (void);
extern uint8_t  queue_length               (const queue *);

extern void     enqueue_atom_X             (uint16_t);
extern uint16_t dequeue_atom_X_NONATOMIC   (void);
extern void     undequeue_atom_X_NONATOMIC (uint16_t);

extern void     enqueue_atom_Y             (uint16_t);
extern uint16_t dequeue_atom_Y_NONATOMIC   (void);
extern void     undequeue_atom_Y_NONATOMIC (uint16_t);

extern void     enqueue_atom_Z             (uint16_t);
extern uint16_t dequeue_atom_Z_NONATOMIC   (void);
extern void     undequeue_atom_Z_NONATOMIC (uint16_t);

extern void     enqueue_atom_P             (uint16_t);
extern uint16_t dequeue_atom_P_NONATOMIC   (void);
extern void     undequeue_atom_P_NONATOMIC (uint16_t);


#if 0
// Implementation

struct queue_private {
    uint8_t q_head;
    uint8_t q_tail;
};

typedef uint16_t queue_buf[256 / 2] __attribute__((aligned(256)));

extern queue_buf Xq_buf, Yq_buf, Zq_buf, Pq_buf;

static inline void init_queues(void)
{
    // Already zeroed.
}

static inline bool queue_is_empty_NONATOMIC(const queue *q)
{
    return q->q_head == q->q_tail;
}

static inline bool queue_is_empty(const queue *q)
{
    bool empty;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        empty = queue_is_empty_NONATOMIC(q);
    }
    return empty;
}

static inline bool queue_is_full_NONATOMIC(const queue *q)
{
    return q->q_head == (uint8_t)(q->q_tail + 2);
}

static inline bool queue_is_full(const queue *q)
{
    bool full;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        full = queue_is_full_NONATOMIC(q);
    }
    return full;
}

static inline bool any_queue_is_full(void)
{
    bool any;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        any = (queue_is_full_NONATOMIC(&Xq) ||
               queue_is_full_NONATOMIC(&Yq) ||
               queue_is_full_NONATOMIC(&Zq) ||
               queue_is_full_NONATOMIC(&Pq));
    }
    return any;
}

static inline uint8_t queue_length(const queue *q)
{
    uint8_t h, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        h = q->q_head;
        t = q->q_tail;
    }
    return t - h;
}

#define DEFINE_ENQUEUE_DEQUEUE(Q)                                       \
                                                                        \
    static inline void enqueue_atom_##Q(uint16_t a)                     \
    {                                                                   \
        union {                                                         \
            uint8_t   b[2];                                             \
            uint16_t *p;                                                \
        } u;                                                            \
                                                                        \
        u.b[1] = (uintptr_t)Q##q_buf >> 8;                              \
        while (queue_is_full(&Q##q))                                    \
            continue;                                                   \
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {                             \
            u.b[0] = Q##q.q_tail;                                       \
            *u.p++ = a;                                                 \
            Q##q.q_tail = u.b[0];                                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static inline uint16_t dequeue_atom_##Q##_NONATOMIC(void)           \
    {                                                                   \
        union {                                                         \
            uint8_t   b[2];                                             \
            uint16_t *p;                                                \
        } u;                                                            \
                                                                        \
        if (queue_is_empty_NONATOMIC(&Q##q))                            \
            return A_STOP;                                              \
        u.b[0] = Q##q.q_head;                                           \
        u.b[1] = (uintptr_t)Q##q_buf >> 8;                              \
        uint16_t a = *u.p++;                                            \
        Q##q.q_head = u.b[0];                                           \
        return a;                                                       \
    }                                                                   \
                                                                        \
    static inline void undequeue_atom_##Q##_NONATOMIC(uint16_t a)       \
    {                                                                   \
        union {                                                         \
            uint8_t   b[2];                                             \
            uint16_t *p;                                                \
        } u;                                                            \
                                                                        \
        fw_assert(!queue_is_full_NONATOMIC(&Q##q));                     \
        u.b[0] = Q##q.q_head - 2;                                       \
        u.b[1] = (uintptr_t)Q##q_buf >> 8;                              \
        *u.p = a;                                                       \
        Q##q.q_head = u.b[0];                                           \
    }

DEFINE_ENQUEUE_DEQUEUE(X);
DEFINE_ENQUEUE_DEQUEUE(Y);
DEFINE_ENQUEUE_DEQUEUE(Z);
DEFINE_ENQUEUE_DEQUEUE(P);

#undef DEFINE_ENQUEUE_DEQUEUE

#endif

#endif /* !QUEUES_included */
