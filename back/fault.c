#include "fault.h"

#include <string.h>

#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "e-stop.h"
#include "fw_assert.h"
#include "illum.h"
#include "variables.h"

typedef uint16_t fault_word;

// N.B.  fault_functions are called both from interrupts and from base level.
typedef void fault_function(void);
typedef fault_function f_func;

typedef struct fault_descriptor {
    PGM_P   fd_name;
    f_func *fd_func;
} fault_descriptor, f_desc;

#define DEFINE_FAULT_NAME(f) static const char fn_##f[] PROGMEM = #f

DEFINE_FAULT_NAME(ES);
DEFINE_FAULT_NAME(LO);
DEFINE_FAULT_NAME(LC);
DEFINE_FAULT_NAME(WF);
DEFINE_FAULT_NAME(WT);
DEFINE_FAULT_NAME(SF);
DEFINE_FAULT_NAME(SO);
DEFINE_FAULT_NAME(SP);
DEFINE_FAULT_NAME(SL);
DEFINE_FAULT_NAME(SS);
DEFINE_FAULT_NAME(SU);
DEFINE_FAULT_NAME(SI);

// XXX Need to add a fault for malfunctioning limit switch.
//     One fault for all limit switches would be fine -- user
//     can easily figure out which switch doesn't work.

static const f_desc fault_descriptors[FAULT_COUNT] PROGMEM = {
    { fn_ES, emergency_stop }, // Emergency Stop
    { fn_LO, NULL           }, // Lid Open
    { fn_LC, NULL           }, // Lid Closed
    { fn_WF, NULL           }, // Water Flow
    { fn_WT, NULL           }, // Water Temperature
    { fn_SF, NULL           }, // Serial Frame Error
    { fn_SO, NULL           }, // Serial Overrun
    { fn_SP, NULL           }, // Serial Parity Error
    { fn_SL, NULL           }, // Software Lexical Error
    { fn_SS, NULL           }, // Software Syntax Error
    { fn_SU, emergency_stop }, // Software Underflow
    { fn_SI, NULL           }, // Software Missed Interrupt
};

static fault_word            fault_states;
static fault_word            fault_overrides;

void get_fault_name(uint8_t findex, f_name *name_out)
{
    fw_assert(findex < FAULT_COUNT);
    PGM_P addr = (PGM_P)pgm_read_word(&fault_descriptors[findex].fd_name);
    strncpy_P(*name_out, addr, sizeof *name_out);
    (*name_out)[sizeof *name_out - 1] = '\0';
}

void clear_all_faults(void)
{
    fault_states = 0;
    fault_overrides = 0;
}

void update_overrides(void)
{
    fault_word new_overrides = 0;
    if (get_enum_variable(V_OO) == 'y')
        new_overrides |= 1 << F_LO;
    if (get_enum_variable(V_OC) == 'y')
        new_overrides |= 1 << F_LC;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fault_overrides = new_overrides;
    }
}

bool fault_is_set(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    return (fault_states & 1 << findex) ? true : false;
}

bool fault_is_overridden(uint8_t findex)
{
    fw_assert(findex < FAULT_COUNT);
    return (fault_overrides & 1 << findex) ? true : false;
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

// N.B.  trigger_fault() is called both from interrupts and from base level.
void trigger_fault(uint8_t findex)
{
    if (fault_is_set(findex))
        return;
    if (fault_is_overridden(findex)) {
        start_animation(A_WARNING);
        return;
    }
    const void *addr = &fault_descriptors[findex].fd_func;
    f_func *f = (f_func *)pgm_read_word(addr);
    if (f) {
        (*f)();
    }
    start_animation(A_ALERT);
}
