#include "atoms.h"

#include "fw_assert.h"

void init_atoms(void)
{
    fw_assert(ATOM_COUNT < 2 * (F_CPU / 1000000L));
}
