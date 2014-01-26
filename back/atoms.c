#include "atoms.h"

#include <stdio.h>

#include <avr/pgmspace.h>

#include "fw_assert.h"

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
DEFINE_ATOM_NAME(A_LASERS_OFF);
DEFINE_ATOM_NAME(A_MAIN_LASER_OFF);
DEFINE_ATOM_NAME(A_MAIN_LASER_ON);
DEFINE_ATOM_NAME(A_MAIN_LASER_START);
DEFINE_ATOM_NAME(A_MAIN_LASER_STOP);
DEFINE_ATOM_NAME(A_VISIBLE_LASER_OFF);
DEFINE_ATOM_NAME(A_VISIBLE_LASER_ON);
DEFINE_ATOM_NAME(A_VISIBLE_LASER_START);
DEFINE_ATOM_NAME(A_VISIBLE_LASER_STOP);
DEFINE_ATOM_NAME(INVALID_ATOM);

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
    A_LASERS_OFF_name,
    A_MAIN_LASER_OFF_name,
    A_MAIN_LASER_ON_name,
    A_MAIN_LASER_START_name,
    A_MAIN_LASER_STOP_name,
    A_VISIBLE_LASER_OFF_name,
    A_VISIBLE_LASER_ON_name,
    A_VISIBLE_LASER_START_name,
    A_VISIBLE_LASER_STOP_name,
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
        fprintf_P(stderr, PSTR("%s %S\n"), label, name);
    } else
        fprintf_P(stderr, PSTR("%s %"PRIu16"\n"), label, a);
}

