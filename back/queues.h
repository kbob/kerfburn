#ifndef QUEUES_included
#define QUEUES_included

#include <stdbool.h>
#include <stdint.h>

#include <util/atomic.h>

#include "atoms.h"
#include "bufs.h"
#include "fw_assert.h"


// Interface

typedef struct queue_private queue;

// Queues for X, Y, Z motors and laser pulses.
extern queue Xq, Yq, Zq, Pq;

static inline void     init_queues                (void);

static inline bool     queue_is_empty             (const queue *);
static inline bool     queue_is_full              (const queue *);
static inline bool     queue_is_empty_NONATOMIC   (const queue *);
static inline bool     queue_is_full_NONATOMIC    (const queue *);
static inline bool     any_queue_is_full          (void);
static inline uint8_t  queue_length               (const queue *);
static inline uint8_t  queue_available            (const queue *);
static inline void     enqueue_atom               (uint16_t, queue *);

static inline void     enqueue_atom_X             (uint16_t);
static inline uint16_t dequeue_atom_X_NONATOMIC   (void);
static inline void     rewind_queue_X_NONATOMIC   (uint8_t count);

static inline void     enqueue_atom_Y             (uint16_t);
static inline uint16_t dequeue_atom_Y_NONATOMIC   (void);
static inline void     rewind_queue_Y_NONATOMIC   (uint8_t count);

static inline void     enqueue_atom_Z             (uint16_t);
static inline uint16_t dequeue_atom_Z_NONATOMIC   (void);
static inline void     rewind_queue_Z_NONATOMIC   (uint8_t count);

static inline void     enqueue_atom_P             (uint16_t);
static inline uint16_t dequeue_atom_P_NONATOMIC   (void);
static inline void     rewind_queue_P_NONATOMIC   (uint8_t count);


// Implementation

typedef uint16_t queue_buf[256 / 2] __attribute__((aligned(256)));

struct queue_private {
    uint8_t   q_head;
    uint8_t   q_tail;
    uint16_t *q_buf;
};

// X, Y, Z, Pq_buf are actually defined in bufs.h.
//extern queue_buf Xq_buf, Yq_buf, Zq_buf, Pq_buf;

static inline void init_queues(void)
{
    Xq.q_buf = Xq_buf;
    Yq.q_buf = Yq_buf;
    Zq.q_buf = Zq_buf;
    Pq.q_buf = Pq_buf;
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
    return (t - h) / sizeof (uint16_t);
}

static inline uint8_t queue_available(const queue *q)
{
    uint8_t h, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        h = q->q_head;
        t = q->q_tail;
    }
    return (uint8_t)(h - t - 2) >> 1;
}

static inline void enqueue_atom(uint16_t a, queue *q)
{
    union {
        uint8_t   b[2];
        uint16_t *p;
    } u;

    u.b[1] = (uintptr_t)q->q_buf >> 8;
    while (queue_is_full(q))
        continue;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        u.b[0] = q->q_tail;
        *u.p++ = a;
        q->q_tail = u.b[0];
    }
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
    static inline void rewind_queue_##Q##_NONATOMIC(uint8_t count)      \
    {                                                                   \
        Q##q.q_head -= count << 1;                                      \
    }

DEFINE_ENQUEUE_DEQUEUE(X);
DEFINE_ENQUEUE_DEQUEUE(Y);
DEFINE_ENQUEUE_DEQUEUE(Z);
DEFINE_ENQUEUE_DEQUEUE(P);

#undef DEFINE_ENQUEUE_DEQUEUE

#endif /* !QUEUES_included */
