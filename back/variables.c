#include "variables.h"

#include <stddef.h>
#include <string.h>

#include <avr/pgmspace.h>

#define MAX_OBSERVERS 1

#define DESC_NAME_OFFSET 0
#define DESC_PUNC_OFFSET 2
#define DESC_TYPE_OFFSET 3
#define DESC_ENUM_OFFSET 4

#define UNSIGNED "\026"
#define SIGNED   "\023"
#define ENUM     "\005"

#define DEFINE_DESC(name, type, ...) \
    static const char name##_desc[] PROGMEM = #name "=" type __VA_ARGS__

DEFINE_DESC(ia, ENUM, "ncswa"); // illumination animation
DEFINE_DESC(il, UNSIGNED);      // illumination level
DEFINE_DESC(lp, UNSIGNED);      // laser power
DEFINE_DESC(ls, ENUM, "nmv");   // laser select
DEFINE_DESC(mt, UNSIGNED);      // move time
DEFINE_DESC(oc, ENUM, "ny");    // override lid closed
DEFINE_DESC(oo, ENUM, "ny");    // override lid open
DEFINE_DESC(pd, UNSIGNED);      // pulse distance
DEFINE_DESC(pi, UNSIGNED);      // pulse interval
DEFINE_DESC(pm, ENUM, "octd");  // pulse mode
DEFINE_DESC(pw, UNSIGNED);      // pulse width
DEFINE_DESC(re, ENUM, "yn");    // report E-Stop status
DEFINE_DESC(rf, ENUM, "yn");    // report fault status
DEFINE_DESC(ri, UNSIGNED);      // reporting interval
DEFINE_DESC(rl, ENUM, "ny");    // report limit switch status
DEFINE_DESC(rm, ENUM, "ny");    // report motor status
DEFINE_DESC(rp, ENUM, "ny");    // report power status
DEFINE_DESC(rq, ENUM, "ny");    // report queue status
DEFINE_DESC(rr, ENUM, "ny");    // report RAM status
DEFINE_DESC(rs, ENUM, "ny");    // report serial status
DEFINE_DESC(rv, ENUM, "ny");    // report variables
DEFINE_DESC(rw, ENUM, "ny");    // report water status
DEFINE_DESC(xd, SIGNED);        // X distance
DEFINE_DESC(yd, SIGNED);        // Y distance
DEFINE_DESC(zd, SIGNED);        // Z distance

static PGM_P const variable_descriptors[VARIABLE_COUNT] PROGMEM = {
    ia_desc,
    il_desc,
    lp_desc,
    ls_desc,
    mt_desc,
    oc_desc,
    oo_desc,
    pd_desc,
    pi_desc,
    pm_desc,
    pw_desc,
    re_desc,
    rf_desc,
    ri_desc,
    rl_desc,
    rm_desc,
    rp_desc,
    rq_desc,
    rr_desc,
    rs_desc,
    rv_desc,
    rw_desc,
    xd_desc,
    yd_desc,
    zd_desc,
};

static v_observ *observers[VARIABLE_COUNT][MAX_OBSERVERS];

struct variables_private variables_private;

static int8_t cmp2(const char *a, const char *b)
{
    if (a[0] > b[0])
        return +1;
    if (a[0] < b[0])
        return -1;
    if (a[1] > b[1])
        return +1;
    if (a[1] < b[1])
        return -1;
    return 0;
}

static PGM_P desc_addr(v_index index)
{
    fw_assert(index < VARIABLE_COUNT);
    return (PGM_P) pgm_read_word(&variable_descriptors[index]);
}

static void notify_observers(v_index index)
{
    for (uint8_t i = 0; i < MAX_OBSERVERS; i++) {
        v_observ *observer = observers[index][i];
        if (!observer)
            break;
        (*observer)(index);
    }
}

void init_variables(void)
{
#ifndef FW_NDEBUG
    v_name name, prev_name;
    size_t max_len = 0;
    for (v_index i = 0; i < VARIABLE_COUNT; i++) {
        get_variable_name(i, &name);
        if (i)
            fw_assert(cmp2(prev_name, name) < 0);
        strcpy(prev_name, name);

        size_t len = strlen_P(desc_addr(i));
        if (max_len < len)
            max_len = len;
    }
    fw_assert(max_len + 1 == VAR_DESC_SIZE);
#endif
    reset_all_variables();
}

void reset_all_variables(void)
{
    for (v_index i = 0; i < VARIABLE_COUNT; i++) {
        v_desc desc;
        v_value value;
        get_variable_desc(i, &desc);
        switch (desc[DESC_TYPE_OFFSET]) {

        case VT_UNSIGNED:
            value.vv_unsigned = 0;
            break;

        case VT_SIGNED:
            value.vv_signed = +0;
            break;

        case VT_ENUM:
            value.vv_enum = desc[DESC_ENUM_OFFSET];
            break;

        default:
            fw_assert(false);
            continue;
        }
        variables_private.vp_values[i] = value;
    }
}

void set_variable(v_index index, v_value value)
{
    fw_assert(index < VARIABLE_COUNT);
    v_value prev = variables_private.vp_values[index];
    variables_private.vp_values[index] = value;
    if (value.vv_unsigned != prev.vv_unsigned) {
        notify_observers(index);
    }
}

v_index lookup_variable(const char *name)
{
    v_index lo = 0, hi = VARIABLE_COUNT;
    while (lo < hi) {
        v_index mid = (lo + hi) / 2;
        v_name mid_name;
        get_variable_name(mid, &mid_name);
        int8_t c = cmp2(name, mid_name);
        if (c == 0)
            return mid;
        if (c < 0)
            hi = mid;
        else
            lo = mid + 1;
    }
    return VAR_NOT_FOUND;
}

void get_variable_desc(v_index index, v_desc *out)
{
    strncpy_P(*out, desc_addr(index), sizeof *out);
    (*out)[sizeof *out - 1] = '\0';
}

void get_variable_name(v_index index, v_name *out)
{
    strncpy_P(*out, desc_addr(index) + DESC_NAME_OFFSET, sizeof *out);
    (*out)[sizeof *out - 1] = '\0';
}

v_type get_variable_type(v_index index)
{
    return pgm_read_byte(desc_addr(index) + DESC_TYPE_OFFSET);
}

bool variable_enum_is_OK(v_index index, char e)
{
    v_desc desc;
    get_variable_desc(index, &desc);
    char *p = desc + DESC_ENUM_OFFSET;
    while (*p)
        if (e == *p++)
            return true;
    return false;
}

void observe_variable(v_index index, v_observ func)
{
    v_observ **p = observers[index];
    uint8_t i;
    for (i = 0; i < MAX_OBSERVERS && p[i]; i++)
        continue;
    fw_assert(i < MAX_OBSERVERS);
    p[i] = func;
}
