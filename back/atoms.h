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
    A_ENABLE_STEP,
    A_DISABLE_STEP,
    A_REWIND_IF_MIN,
    A_REWIND_UNLESS_MIN,
    A_REWIND_IF_MAX,
    A_REWIND_UNLESS_MAX,

    // Atoms for lasers
    A_LASERS_OFF,

    A_MAIN_LASER_OFF,
    A_MAIN_LASER_ON,
    A_MAIN_LASER_START,
    A_MAIN_LASER_STOP,

    A_VISIBLE_LASER_OFF,
    A_VISIBLE_LASER_ON,
    A_VISIBLE_LASER_START,
    A_VISIBLE_LASER_STOP,

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
