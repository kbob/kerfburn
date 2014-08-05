#include "fault.h"

#include <string.h>

#include <avr/pgmspace.h>
#include <util/atomic.h>

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
DEFINE_FAULT_NAME(LS);
DEFINE_FAULT_NAME(SF);
DEFINE_FAULT_NAME(SO);
DEFINE_FAULT_NAME(SP);
DEFINE_FAULT_NAME(SL);
DEFINE_FAULT_NAME(SS);
DEFINE_FAULT_NAME(SU);
DEFINE_FAULT_NAME(SI);

static const f_desc fault_descriptors[FAULT_COUNT] PROGMEM = {
    { fn_ES, NULL },            // Emergency Stop
    { fn_LO, NULL },            // Lid Open
    { fn_LC, NULL },            // Lid Closed
    { fn_WF, NULL },            // Water Flow
    { fn_WT, NULL },            // Water Temperature
    { fn_LS, NULL },            // Limit Switch Stuck
    { fn_SF, NULL },            // Serial Frame Error
    { fn_SO, NULL },            // Serial Overrun
    { fn_SP, NULL },            // Serial Parity Error
    { fn_SL, NULL },            // Software Lexical Error
    { fn_SS, NULL },            // Software Syntax Error
    { fn_SU, NULL },            // Software Underflow
    { fn_SI, NULL },            // Software Missed Interrupt
};

struct fault_private fault_private;

// N.B.  raise_fault() is called both from interrupts and from base level.
void raise_fault(fault_index findex)
{
    if (fault_is_set(findex))
        return;
    set_fault(findex);
    const void *addr = &fault_descriptors[findex].fd_func;
    f_func *f = (f_func *)pgm_read_word(addr);
    if (f) {
        (*f)();
    }
    start_animation(A_ALERT);
}

void lower_fault(fault_index findex)
{
    if (!fault_is_set(findex))
        return;
    clear_fault(findex);
    const void *addr = &fault_descriptors[findex].fd_func;
    f_func *f = (f_func *)pgm_read_word(addr);
    if (f) {
        (*f)();
    }
    if (!fault_private.fault_states)
        start_animation(A_NONE);
}

void lower_all_faults(void)
{
    for (f_index f = 0; f < FAULT_COUNT; f++)
        if (fault_is_set(f))
            lower_fault(f);
}

void get_fault_name(fault_index findex, f_name *name_out)
{
    fw_assert(findex < FAULT_COUNT);
    PGM_P addr = (PGM_P)pgm_read_word(&fault_descriptors[findex].fd_name);
    strncpy_P(*name_out, addr, sizeof *name_out);
    (*name_out)[sizeof *name_out - 1] = '\0';
}
