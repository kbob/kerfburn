#ifndef FAULT_included
#define FAULT_included

#include <stdbool.h>
#include <stdint.h>

#define FAULT_NAME_SIZE 3

enum fault_index {
    F_ES =  0,                  // Emergency Stop
    F_LO =  1,                  // Lid Open
    F_LC =  2,                  // Lid Closed
    F_WF =  3,                  // Water Flow
    F_WT =  4,                  // Water Temperature
    F_SF =  5,                  // Serial Frame Error
    F_SO =  6,                  // Serial Overrun
    F_SP =  7,                  // Serial Parity Error
    F_SL =  8,                  // Software Lexical Error
    F_SS =  9,                  // Software Syntax Error
    F_SU = 10,                  // Software Underflow
    F_SI = 11,                  // Software Missed Interrupt
    FAULT_COUNT
} fault_index, f_index;

typedef char fault_name[FAULT_NAME_SIZE];
typedef fault_name f_name;

// XXX inline all except trigger_fault() and update_overrides().
extern void clear_all_faults     (void);
extern void update_overrides     (void);
extern bool fault_is_set         (uint8_t findex);
extern bool fault_is_overridden  (uint8_t findex);
extern void get_fault_name       (uint8_t findex, f_name *name_out);

// set_fault simply sets the flag.  trigger_fault sets the flag and
// also does associated actions.  (Stop motors, change illumination,
// etc.)
extern void trigger_fault        (uint8_t findex);

extern void set_fault            (uint8_t findex);
extern void clear_fault          (uint8_t findex);
extern void set_fault_override   (uint8_t findex, bool override);
extern void clear_fault_override (uint8_t findex, bool override);

#endif /* !FAULT_included */
