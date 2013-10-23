#ifndef ATOMS_included
#define ATOMS_included

#include <stdbool.h>
#include <stdint.h>

// Each timer's queue has 128 entries.  Each entry is 16 bits.  I'm
// calling those 16 bit entries _atoms_.  An atom that is 32 or more
// is a value to be loaded into the comparator register.  An atom that
// is less than 32 is one of these enumerated constants.

typedef enum atom {

    A_STOP = 0,

    // Atoms for motors
    A_DIR_POSITIVE,
    A_DIR_NEGATIVE,
    A_LOOP_UNTIL_MIN,
    A_LOOP_WHILE_MIN,
    A_LOOP_UNTIL_MAX,
    A_LOOP_WHILE_MAX,
    A_ENABLE_STEP,
    A_DISABLE_STEP,

    // Atoms for lasers
    A_SET_MAIN_LASER_OFF,
    A_SET_MAIN_LASER_PULSED,
    A_SET_MAIN_LASER_CONTINUOUS,
    A_SET_VISIBLE_LASER_OFF,
    A_SET_VISIBLE_LASER_PULSED,
    A_SET_VISIBLE_LASER_CONTINUOUS,

    A_SET_MAIN_PULSE_DURATION,    // next atom is value.
    A_SET_VISIBLE_PULSE_DURATION, // next atom is value.
    A_SET_MAIN_POWER_LEVEL,       // next atom is value.

    ATOM_COUNT,
    INVALID_ATOM = ATOM_COUNT

} atom;

#define ATOM_MAX 32

extern void init_atoms(void);
extern void print_atom(const char *label, uint16_t a);

static inline bool is_atom(uint16_t a)
{
    return a < ATOM_MAX;
}

#endif /* !ATOMS_included */
