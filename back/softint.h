#ifndef SOFTINT_included
#define SOFTINT_included

#include <stdbool.h>
#include <stdint.h>

#include <util/atomic.h>

// Software interrupt is intermediate in priority between hardware
// interrupt and base-level (non-interrupt) code.  A software interrupt
// always preempts base-level code, and a hardware interrupt preempts
// a software interrupt.  Software interrupts do not preempt one
// another.
//
// Trigger a software interrupt with one of the trigger_softint_*
// functions, depending on the caller's priority.
//
// The trigger_softint_* functions take an argument, task, which
// indicates what the software interrupt handler should do when it
// runs.
//
// Any interrupt service routine that may trigger a software interrupt
// should be declared with the ISR_TRIGGERS_SOFTINT macro.

typedef enum softint_task {
    ST_STEPGEN = 1 << 0,
    ST_TIMER   = 1 << 1,
} softint_task;

static inline void trigger_softint_from_base    (uint8_t task);
static inline void trigger_softint_from_softint (uint8_t task);
static inline void trigger_softint_from_hardint (uint8_t task);

#define ISR_TRIGGERS_SOFTINT(vector)                            \
    void f_##vector(void);                                      \
    __attribute__((signal, naked, used, externally_visible))    \
    void vector(void)                                           \
    {                                                           \
        __asm__ volatile("push   r1\n\t"                        \
                         "push   r0\n\t"                        \
                         "in     r0, __SREG__\n\t"              \
                         "push   r0\n\t"                        \
                         "clr    r1\n\t"                        \
                         "push   r18\n\t"                       \
                         "push   r19\n\t"                       \
                         "push   r20\n\t"                       \
                         "push   r21\n\t"                       \
                         "push   r22\n\t"                       \
                         "push   r23\n\t"                       \
                         "push   r24\n\t"                       \
                         "push   r25\n\t"                       \
                         "push   r26\n\t"                       \
                         "push   r27\n\t"                       \
                         "push   r30\n\t"                       \
                         "push   r31");                         \
                                                                \
        f_##vector();                                           \
        softint_private.is_active = true;                       \
        sei();                                                  \
        softint_dispatch();                                     \
                                                                \
        __asm__ volatile("pop    r31\n\t"                       \
                         "pop    r30\n\t"                       \
                         "pop    r27\n\t"                       \
                         "pop    r26\n\t"                       \
                         "pop    r25\n\t"                       \
                         "pop    r24\n\t"                       \
                         "pop    r23\n\t"                       \
                         "pop    r22\n\t"                       \
                         "pop    r21\n\t"                       \
                         "pop    r20\n\t"                       \
                         "pop    r19\n\t"                       \
                         "pop    r18\n\t"                       \
                         "pop    r0\n\t"                        \
                         "out    __SREG__, r0\n\t"              \
                         "pop    r0\n\t"                        \
                         "pop    r1\n\t"                        \
                         "reti");                               \
    }                                                           \
    void f_##vector(void)

extern struct softint_private {
    bool    is_active;
    uint8_t pending_tasks;
} softint_private;

extern void softint_dispatch(void); // private

// trigger_softint_from_base()
//
// Assume that interrupts are enabled and that we are called from base
// level.  Also assume that no soft interrupts are already pending.
// (Otherwise we would already have serviced them.)

static inline void trigger_softint_from_base(uint8_t task)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        softint_private.pending_tasks = task;
        softint_private.is_active = true;
    }
    softint_dispatch();
}

// trigger_softint_from_softint()
//
// Simply set the task flag.  The dispatcher will see it next time
// through.

static inline void trigger_softint_from_softint(uint8_t task)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        softint_private.pending_tasks |= task;
    }
}

// trigger_softint_from_hardint()
//
// When called, we do not know whether the soft interrupt is active.
// We simply set the task flag here.  When this ISR returns, because
// it was declared with ISR_TRIGGERS_SOFTINT, the soft interrupt will
// be delivered.

static inline void trigger_softint_from_hardint(uint8_t task)
{
    softint_private.pending_tasks |= task;
}

#endif /* !SOFTINT_included */
