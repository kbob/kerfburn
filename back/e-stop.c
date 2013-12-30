#include "e-stop.h"

#include "config/pin-defs.h"

#include "fw_assert.h"
#include "pin-io.h"

void init_emergency_stop(void)
{
}

void emergency_stop_NONATOMIC(void)
{
    fw_assert(0);
}

void emergency_stop(void)
{
    fw_assert(0);
}

bool is_emergency_stopped(void)
{
    return false;
    // return REG_BIT_IS(EMERGENCY_STOP_PORT, EMERGENCY_STOP_STOPPED);
}
