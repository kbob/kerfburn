#ifndef VARIABLES_included
#define VARIABLES_included

#include <stdbool.h>
#include <stdint.h>

#include "fw_assert.h"

// Interface

#define VAR_NOT_FOUND 0xFF      // returned by lookup_variable()
#define VAR_NAME_SIZE    3      // name size, including NUL byte
#define VAR_DESC_SIZE   10      // descriptor size, including NUL byte

typedef enum variable_index {
    V_IA,                       // illumination animation
    V_IL,                       // illumination level
    V_LM,                       // laser mode
    V_LP,                       // laser power
    V_LS,                       // laser select
    V_MT,                       // move time
    V_OC,                       // override lid closed
    V_OO,                       // override lid open
    V_PD,                       // pulse distance
    V_PI,                       // pulse interval
    V_PL,                       // pulse length (duration)
    V_RE,                       // report E-Stop status
    V_RF,                       // report fault status
    V_RI,                       // reporting interval (milliseconds)
    V_RL,                       // report limit switch status
    V_RM,                       // report motor status
    V_RP,                       // report power status
    V_RQ,                       // report queue status
    V_RR,                       // report RAM status
    V_RS,                       // report serial status
    V_RV,                       // report variables
    V_RW,                       // report water status
    V_XD,                       // X distance
    V_YD,                       // Y distance
    V_ZD,                       // Z distance
    VARIABLE_COUNT
} variable_index, v_index;

typedef enum variable_type {
    VT_UNSIGNED = 026,
    VT_SIGNED   = 023,
    VT_ENUM     = 005,
} variable_type, v_type;

typedef union variable_value {
    uint32_t    vv_unsigned;
    int32_t     vv_signed;
    uint32_t    vv_enum;
} variable_value, v_value;

typedef char                variable_name      [VAR_NAME_SIZE];
typedef variable_name       v_name;
typedef char                variable_descriptor[VAR_DESC_SIZE];
typedef variable_descriptor v_desc;

extern        void     init_variables        (void);
extern        void     reset_all_variables   (void);

// Reflection interface
extern        uint8_t  lookup_variable       (const char *name);
extern        void     get_variable_desc     (uint8_t index, v_desc *out);
extern        void     get_variable_name     (uint8_t index, v_name *out);
extern        v_type   get_variable_type     (uint8_t index);
extern        bool     variable_enum_is_OK   (uint8_t index, char e);

static inline v_value  get_variable          (uint8_t index);
static inline uint32_t get_unsigned_variable (uint8_t index);
static inline int32_t  get_signed_variable   (uint8_t index);
static inline uint8_t  get_enum_variable     (uint8_t index);

static inline void     set_variable          (uint8_t, v_value);
static inline void     set_unsigned_variable (uint8_t index, uint32_t);
static inline void     set_signed_variable   (uint8_t index, int32_t);
static inline void     set_enum_variable     (uint8_t index, uint8_t);

// Implementation

extern struct variables_private {
    v_value vp_values[VARIABLE_COUNT];
} variables_private;

static inline v_value get_variable(uint8_t index)
{
    fw_assert(index < VARIABLE_COUNT);
    return variables_private.vp_values[index];
}

static inline uint32_t get_unsigned_variable(uint8_t index)
{
    return get_variable(index).vv_unsigned;
}

static inline int32_t get_signed_variable(uint8_t index)
{
    return get_variable(index).vv_signed;
}

static inline uint8_t get_enum_variable(uint8_t index)
{
    return get_variable(index).vv_enum;
}

static inline void set_variable(uint8_t index, v_value value)
{
    fw_assert(index < VARIABLE_COUNT);
    variables_private.vp_values[index] = value;
}

static inline void set_unsigned_variable(uint8_t index, uint32_t u)
{
    v_value v;
    v.vv_unsigned = u;
    set_variable(index, v);
}

static inline void set_signed_variable(uint8_t index, int32_t s)
{
    v_value v;
    v.vv_signed = s;
    set_variable(index, v);
}

static inline void set_enum_variable(uint8_t index, uint8_t e)
{
    v_value v;
    v.vv_enum = e;
    set_variable(index, v);
}

#endif /* !VARIABLES_included */
