#include "fault.h"

#include <string.h>

#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "e-stop.h"
#include "fw_assert.h"
#include "illum.h"

typedef uint16_t fault_word;
typedef void fault_function(void);
typedef fault_function f_func;

static fault_word            fault_states;
static fault_word            fault_overrides;
static f_func PROGMEM *const fault_functions[FAULT_COUNT] = {
    emergency_stop,             // F_ESTOP
    NULL,                       // F_LID_OPEN
    NULL,                       // F_LID_CLOSED
    NULL,                       // F_WATER_FLOW
    NULL,                       // F_WATER_TEMP
    NULL,                       // F_SERIAL_FRAME
    NULL,                       // F_SERIAL_OVERRUN
    NULL,                       // F_SERIAL_PARITY
    NULL,                       // F_SW_LEXICAL
    NULL,                       // F_SW_SYNTAX
    NULL,                       // F_SW_UNDERFLOW
    NULL,                       // F_SW_MISSED_INTR
};

void clear_all_faults(void)
{
    fault_states = 0;
    fault_overrides = 0;
}

bool fault_is_set(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    return (fault_states & 1 << findex) ? true : false;
}

bool fault_is_overridden(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    return (fault_states & 1 << findex) ? true : false;
}

void set_fault(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fault_states |= 1 << findex;
    }
}

void clear_fault(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fault_states &= ~findex;
    }
}

void trigger_fault(uint8_t findex)
{
    if (fault_is_set(findex))
        return;
    if (fault_is_overridden(findex)) {
        start_animation(A_WARNING);
        return;
    }
    const void *addr = &fault_functions[findex];
    f_func *f = (f_func *)pgm_read_word(addr);
    if (f) {
        (*f)();
    }
    start_animation(A_ALERT);
}
