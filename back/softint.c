#include "softint.h"

#include "timer.h"

struct softint_private softint_private;

void softint_dispatch(void)
{
    while (true) {
        uint8_t tasks;
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            tasks = softint_private.pending_tasks;
            softint_private.pending_tasks = 0;
            if (tasks == 0)
                softint_private.is_active = false;
        }
        if (tasks == 0)
            break;
#if 0 // not defined yet.
        if (tasks & ST_STEPGEN)
            stepgen_softint();
#endif
        if (tasks & ST_TIMER)
            timer_softint();
    }
}
