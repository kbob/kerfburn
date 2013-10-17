#include "atoms.h"

#include <stdio.h>

#include <avr/pgmspace.h>

#include "fw_assert.h"
#include "fw_stdio.h"

#define DEFINE_ATOM_NAME(A) static const char A##_name[] PROGMEM = #A;

DEFINE_ATOM_NAME(A_STOP);
DEFINE_ATOM_NAME(A_DIR_POSITIVE);
DEFINE_ATOM_NAME(A_DIR_NEGATIVE);
DEFINE_ATOM_NAME(A_LOOP_UNTIL_MIN);
DEFINE_ATOM_NAME(A_LOOP_WHILE_MIN);
DEFINE_ATOM_NAME(A_LOOP_UNTIL_MAX);
DEFINE_ATOM_NAME(A_LOOP_WHILE_MAX);
DEFINE_ATOM_NAME(A_ENABLE_STEP);
DEFINE_ATOM_NAME(A_DISABLE_STEP);
DEFINE_ATOM_NAME(A_SET_MAIN_MODE_OFF);
DEFINE_ATOM_NAME(A_SET_MAIN_MODE_PULSED);
DEFINE_ATOM_NAME(A_SET_MAIN_MODE_CONTINUOUS);
DEFINE_ATOM_NAME(A_SET_VISIBLE_MODE_OFF);
DEFINE_ATOM_NAME(A_SET_VISIBLE_MODE_PULSED);
DEFINE_ATOM_NAME(A_SET_VISIBLE_MODE_CONTINUOUS);
DEFINE_ATOM_NAME(A_SET_MAIN_PULSE_DURATION);
DEFINE_ATOM_NAME(A_SET_VISIBLE_PULSE_DURATION);
DEFINE_ATOM_NAME(A_SET_MAIN_POWER_LEVEL);

static const char *const atom_names[ATOM_COUNT] PROGMEM = {
    A_STOP_name,
    A_DIR_POSITIVE_name,
    A_DIR_NEGATIVE_name,
    A_LOOP_UNTIL_MIN_name,
    A_LOOP_WHILE_MIN_name,
    A_LOOP_UNTIL_MAX_name,
    A_LOOP_WHILE_MAX_name,
    A_ENABLE_STEP_name,
    A_DISABLE_STEP_name,
    A_SET_MAIN_MODE_OFF_name,
    A_SET_MAIN_MODE_PULSED_name,
    A_SET_MAIN_MODE_CONTINUOUS_name,
    A_SET_VISIBLE_MODE_OFF_name,
    A_SET_VISIBLE_MODE_PULSED_name,
    A_SET_VISIBLE_MODE_CONTINUOUS_name,
    A_SET_MAIN_PULSE_DURATION_name,
    A_SET_VISIBLE_PULSE_DURATION_name,
    A_SET_MAIN_POWER_LEVEL_name,
};

void init_atoms(void)
{
    fw_assert(ATOM_MAX == 2 * (F_CPU / 1000000L));
    fw_assert(ATOM_COUNT < ATOM_MAX);
}

void print_atom(const char *label, uint16_t a)
{
    if (a < ATOM_COUNT) {
        const char *name = (PGM_P)pgm_read_word(&atom_names[a]);
        fprintf_P(stderr, PSL("%s %S\n"), label, name);
    } else
        fprintf_P(stderr, PSL("%s %"PRIu16"\n"), label, a);
}

