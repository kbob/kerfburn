#ifndef PARAM_included
#define PARAM_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum param_index {
    P_DT = 0,                   // dwell time
    P_IA = 1,                   // illumination animation
    P_IL = 2,                   // illumination level
    P_LM = 3,                   // laser mode
    P_LP = 4,                   // laser power
    P_LS = 5,                   // laser select
    P_PD = 6,                   // pulse distance
    P_PI = 7,                   // pulse interval
    P_PL = 8,                   // pulse length (duration)
    P_X0 = 9,                   // X initial
    P_XA = 10,                  // X acceleration
    P_XD = 11,                  // X distance
    P_Y0 = 12,                  // Y initial
    P_YA = 13,                  // Y acceleration
    P_YD = 14,                  // Y distance
    PARAM_COUNT
} param_index;

typedef enum param_type {
    PT_SIGNED,
    PT_UNSIGNED,
    PT_ENUM
} param_type;

typedef union param_value {
    int32_t  pv_signed;
    uint32_t pv_unsigned;
    int32_t  pv_enum;
} param_value;

typedef struct param_descriptor {
    const char        p_name[3];
    const uint8_t     p_type;   // a param_type
    const param_value p_default_value;
} param_descriptor;

extern const param_descriptor param_descriptors[PARAM_COUNT];
extern param_value param_values[PARAM_COUNT];

extern param_index lookup_param(const char *name);
extern bool        param_enum_is_OK(param_index param, char value);
extern void        assign_param(param_index param, param_value value);

#endif /* !PARAM_included */
