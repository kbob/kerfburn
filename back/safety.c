#include "safety.h"

#include <avr/interrupt.h>

#include "config/pin-defs.h"

#include "fw_assert.h"
#include "pin-io.h"

struct safety_private safety_private;

static safety_callback *callback;

void init_safety(void)
{
    INIT_INPUT_PIN(LID);
    INIT_INPUT_PIN(EMERGENCY_STOP);
}

void set_safety_callback(safety_callback new_callback)
{
    fw_assert(!callback);
    new_callback = callback;
}

void update_safety(void)
{
}

#if LID_grp != EMERGENCY_STOP_grp
ISR(LID_vect, ISR_ALIASOF(EMERGENCY_STOP_vect))
#else
ISR(EMERGENCY_STOP_vect)
#endif
{
    bool open = REG_BIT_IS(LID_PIN, LID_OPEN);
    bool stopped = REG_BIT_IS(EMERGENCY_STOP_PIN, EMERGENCY_STOP_STOPPED);
    uint8_t state = (stopped ? stop_switch : 0) | (open ? lid_switch : 0);
    if (state != safety_private.state) {
        safety_private.state = state;
        if (callback)
            (*callback)();
    }
}
