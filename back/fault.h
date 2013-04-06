#ifndef FAULT_included
#define FAULT_included

#include <stdbool.h>
#include <stdint.h>

enum fault_index {
    F_ESTOP          =  0,
    F_LID_OPEN       =  1,
    F_LID_CLOSED     =  2,
    F_WATER_FLOW     =  3,
    F_WATER_TEMP     =  4,
    F_SERIAL_FRAME   =  5,
    F_SERIAL_OVERRUN =  6,
    F_SERIAL_PARITY  =  7,
    F_SW_LEXICAL     =  8,
    F_SW_SYNTAX      =  9,
    F_SW_UNDERFLOW   = 10,
    F_SW_MISSED_INTR = 11,
    FAULT_COUNT
} fault_index, f_index;

// XXX inline all except trigger_fault().
extern void clear_all_faults     (void);
extern bool fault_is_set         (uint8_t findex);
extern bool fault_is_overridden  (uint8_t findex);

// set_fault simply sets the flag.  trigger_fault sets the flag and
// also does associated actions.  (Stop motors, change illumination,
// etc.)
extern void trigger_fault        (uint8_t findex);

extern void set_fault            (uint8_t findex);
extern void clear_fault          (uint8_t findex);
extern void set_fault_override   (uint8_t findex, bool override);
extern void clear_fault_override (uint8_t findex, bool override);

#endif /* !FAULT_included */
