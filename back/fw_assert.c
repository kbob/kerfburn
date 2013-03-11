#include "fw_assert.h"

#include <stdlib.h>

__attribute__((noreturn))
extern void fw_assertion_failed(int line_no)
{
    abort();
}

