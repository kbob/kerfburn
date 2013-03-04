#include "softint.h"

#include <stdbool.h>

#include <util/atomic.h>

static uint8_t softint_pending_tasks;
static bool softint_active;

void softint_dispatch(void)
{
    while (true) {
        uint8_t tasks;
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            tasks = softint_pending_tasks;
            softint_pending_tasks = 0;
            if (tasks == 0)
                softint_active = false;
        }
        if (tasks == 0)
            return;
#if 0 // not defined yet.
        if (tasks & ST_STEPGEN)
            stepgen_softint();
        if (tasks & ST_TIMER)
            timer_softint();
#endif
    }
}

// trigger_softint_from_base()
//
// Assume that interrupts are enabled and that we are called from base
// level.  Also assume that no soft interrupts are already pending.
// (Otherwise we would already have serviced them.)

void trigger_softint_from_base(uint8_t task)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        softint_pending_tasks = task;
        softint_active = true;
    }
    softint_dispatch();
}

// trigger_softint_from_softint()
//
// Simply set the task flag.  The dispatcher will see it next time
// through.

void trigger_softinit_from_softint(uint8_t task)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        softint_pending_tasks |= task;
    }
}

// trigger_softint_from_hardint()
//
// When called, we do not know whether the soft interrupt is active.
// We simply set the task flag here.  When this ISR returns, because
// it was declared with ISR_TRIGGERS_SOFTINT, the soft interrupt will
// be delivered.

void trigger_softint_from_hardint(uint8_t task)
{
    softint_pending_tasks |= task;
}
