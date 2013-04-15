#include "timer.h"

#include <stddef.h>

#include "fw_assert.h"
#include "softint.h"

struct timer_private timer_private;
static timeout *timeout_queue;

static inline int8_t compare_times(uint32_t t0, uint32_t t1)
{
    int32_t t0s = t0;
    int32_t t1s = t1;
    int32_t d = t0s - t1s;
    return d > 0 ? +1 : (d < 0 ? -1 : 0);
}

void init_timer(void)
{
    // Timer/Counter 0 waveform generation mode = fast PWM, TOP = OCR0A.
    TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(WGM02);

    // Timer/Counter 0 clock select = prescale by 64.
    TCCR0B = _BV(CS00) | _BV(CS01);

    // Output Compare Register 0A = 250 (1 millisecond);
    #if F_CPU % (64 * 1000) || F_CPU / 64 / 5000 >= 256
        #error F_CPU does not work with prescale 64
    #endif
    OCR0A = F_CPU / 64 / 1000;

    // Enable Timer/Counter 0 overflow interrupt.
    TIMSK0 = _BV(TOIE0);
}

static bool dequeue_timeout_NONATOMIC(timeout *newt)
{
    timeout *p, **pp;

    // Remove if already enqueued.
    for (pp = &timeout_queue; (p = *pp); pp = &p->to_next) {
        if (p == newt) {
            *pp = p->to_next;
            return true;
        }
    }
    return false;
}

void enqueue_timeout(timeout *newt, uint32_t expiration)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

        timeout *p, **pp;

        // Dequeue if already enqueued.
        dequeue_timeout_NONATOMIC(newt);

        // Find spot and insert.
        for (pp = &timeout_queue; (p = *pp); pp = &p->to_next)
            if (compare_times(p->to_expiration, expiration) > 0)
                break;
        newt->to_expiration = expiration;
        newt->to_next = p;
        *pp = newt;
    }
}

void dequeue_timeout(timeout *newt)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bool ok = dequeue_timeout_NONATOMIC(newt);
        fw_assert(ok);
    }
}

void timer_softint(void)
{
    while (true) {
        timeout *head = NULL;
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            if (timeout_queue &&
                compare_times(millisecond_time_NONATOMIC(),
                              timeout_queue->to_expiration) >= 0) {
                head = timeout_queue;
                timeout_queue = head->to_next;
            }
        }
        if (!head)
            break;
        if (head->to_interval) {
            head->to_expiration += head->to_interval;
            enqueue_timeout(head, head->to_expiration);
        }
        (*head->to_func)();
    }        
}

ISR_TRIGGERS_SOFTINT(TIMER0_OVF_vect)
{
    timer_private.ticks++;
    if (timeout_queue &&
        compare_times(timer_private.ticks, timeout_queue->to_expiration) > 0) {
        trigger_softint_from_hardint(ST_TIMER);
    }
}
