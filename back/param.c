#include "param.h"

#include <stddef.h>

#include "fw_assert.h"

const param_descriptor param_descriptors[PARAM_COUNT] = {
    { "dt", PT_UNSIGNED, {  0  } }, // dwell time
    { "ia", PT_ENUM,     { 'o' } }, // illumination animation
    { "il", PT_UNSIGNED, {  0  } }, // illumination level
    { "lm", PT_ENUM,     { 'o' } }, // laser mode
    { "lp", PT_UNSIGNED, {  0  } }, // laser power
    { "ls", PT_ENUM,     { 'n' } }, // laser select
    { "pd", PT_UNSIGNED, {  0  } }, // pulse distance
    { "pi", PT_UNSIGNED, {  0  } }, // pulse interval
    { "pl", PT_UNSIGNED, {  0  } }, // pulse length (duration)
    { "x0", PT_UNSIGNED, {  0  } }, // x initial
    { "xa", PT_SIGNED,   {  0  } }, // x acceleration
    { "xd", PT_SIGNED,   {  0  } }, // x distance
    { "y0", PT_UNSIGNED, {  0  } }, // y initial
    { "ya", PT_SIGNED,   {  0  } }, // y acceleration
    { "yd", PT_SIGNED,   {  0  } }, // y distance
};

xparam_index lookup_param(const char *name)
{
    uint8_t lo = 0, hi = PARAM_COUNT;
    while (lo < hi) {
        uint8_t mid = (lo + hi) / 2;
        int8_t delta;
        const char *p = param_descriptors[mid].p_name;
        delta = name[0] - p[0];
        if (delta == 0)
            delta = (name[1] == '=' ? '\0' : name[1]) - p[1];
        if (delta > 0)
            lo = mid + 1;
        else if (delta < 0)
            hi = mid;
        else
            return (param_index)mid;
    }
    return -1;
}

bool param_enum_is_OK(param_index param, char value)
{
    const char *legal;
    if (param == P_IA)
        legal = "scwan";
    else if (param == P_LM)
        legal = "cdto";
    else if (param == P_LS)
        legal = "mvn";
    else
        return false;
    while (*legal)
        if (value == *legal++)
            return true;
    return false;
}

void assign_param(param_index param, param_value value)
{
    fw_assert(param < PARAM_COUNT);
    param_values[param] = value;
}
