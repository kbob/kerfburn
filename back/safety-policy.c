#include "safety-policy.h"

#include "fw_assert.h"

void init_safety_policy(void)
{}

void emergency_stop(void)
{
    fw_assert(false);
}

bool is_emergency_stopped(void)
{
    return false;
}
