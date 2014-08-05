#ifndef FAULT_included
#define FAULT_included

#include <stdbool.h>
#include <stdint.h>

#include "fw_assert.h"

// Interface

#define FAULT_NAME_SIZE 3

typedef enum fault_index {
    F_ES =  0,                  // Emergency Stop
    F_LO =  1,                  // Lid Open
    F_LC =  2,                  // Lid Closed
    F_WF =  3,                  // Water Flow
    F_WT =  4,                  // Water Temperature
    F_LS =  5,                  // Limit Switch Stuck
    F_SF =  6,                  // Serial Frame Error
    F_SO =  7,                  // Serial Overrun
    F_SP =  8,                  // Serial Parity Error
    F_SL =  9,                  // Software Lexical Error
    F_SS = 10,                  // Software Syntax Error
    F_SU = 11,                  // Software Underflow
    F_SI = 12,                  // Software Missed Interrupt
    FAULT_COUNT
} fault_index;

typedef char fault_name[FAULT_NAME_SIZE];

typedef fault_index f_index;
typedef fault_name  f_name;

// set_fault() and clear_fault() simply set and clear the fault bit.
// raise_fault() and lower_fault() set/clear the bit and also perform
// associated actions.  (Stop motors, change illumination, etc.)

extern        void raise_fault      (fault_index findex);
extern        void lower_fault      (fault_index findex);
extern        void lower_all_faults (void);

static inline void set_fault        (fault_index findex);
static inline void clear_fault      (fault_index findex);
static inline void clear_all_faults (void);
static inline bool fault_is_set     (fault_index findex);

// Reflection interface
extern        void get_fault_name   (fault_index findex, f_name *name_out);


// Implementation

typedef uint16_t fault_word;
extern struct fault_private {
    uint16_t fault_states;
} fault_private;

static inline void set_fault        (fault_index findex)
{
    fw_assert(findex < FAULT_COUNT);
    fault_private.fault_states |= 1 << findex;
}

static inline void clear_fault      (fault_index findex)
{
    fw_assert(findex < FAULT_COUNT);
    fault_private.fault_states &= ~(1 << findex);
}

static inline void clear_all_faults (void)
{
    fault_private.fault_states = 0;
}

static inline bool fault_is_set     (fault_index findex)
{
    fw_assert(findex < FAULT_COUNT);
    return (fault_private.fault_states & 1 << findex) ? true : false;
}

#endif /* !FAULT_included */
