#ifndef TIMER_included
#define TIMER_included

#include <stdint.h>

#include <avr/interrupt.h>
#include <util/atomic.h>

typedef void (*timeout_func)(void);

typedef struct timeout {
    struct timeout *to_next;
    uint32_t        to_expiration;
    timeout_func    to_func;
} timeout;

extern        void     init_timer                 (void);
static inline uint32_t millisecond_time_NONATOMIC (void);
static inline uint32_t millisecond_time           (void);
extern        void     enqueue_timeout            (timeout *,
                                                   uint32_t expiration);
extern        void     timer_softint              (void);

extern struct timer_private {
    uint32_t ticks;
} timer_private;

static inline uint32_t millisecond_time_NONATOMIC(void)
{
    return timer_private.ticks;
}

static inline uint32_t millisecond_time(void)
{
    uint32_t now;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        now = millisecond_time_NONATOMIC();
    }
    return now;
}

#endif /* !TIMER_included */
