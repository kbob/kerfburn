#ifndef ATOMS_included
#define ATOMS_included

// Each timer's queue has 128 entries.  Each entry is 16 bits.  I'm
// calling those 16 bit entries _atoms_.  An atom that is 32 or more
// is a value to be loaded into the comparator register.  An atom that
// is less than 32 is one of these enumerated constants.

typedef enum atom {
    A_STOP = 0,
    A_DIR_POSITIVE,
    A_DIR_NEGATIVE,
    A_LOOP_UNTIL_MIN,
    A_LOOP_WHILE_MIN,
    A_LOOP_UNTIL_MAX,
    A_LOOP_WHILE_MAX,
    A_ENABLE_PIN,               // motor step or main laser pulse
    A_DISABLE_PIN,
    A_ENABLE_ALT_PIN,           // visible laser pulse
    A_DISABLE_ALT_PIN,
} atom;

#endif /* !ATOMS_included */
