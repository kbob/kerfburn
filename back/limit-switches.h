#ifndef LIMIT_SWITCHES_included
#define LIMIT_SWITCHES_included

#include <avr/io.h>

#include "config/pin-defs.h"

#include "pin-io.h"

static inline void init_limit_switches(void)
{
#ifdef X_MIN_SWITCH
    INIT_INPUT_PIN(X_MIN_SWITCH);
#endif
#ifdef X_MAX_SWITCH
    INIT_INPUT_PIN(X_MAX_SWITCH);
#endif

#ifdef Y_MIN_SWITCH
    INIT_INPUT_PIN(Y_MIN_SWITCH);
#endif
#ifdef Y_MAX_SWITCH
    INIT_INPUT_PIN(Y_MAX_SWITCH);
#endif

#ifdef Z_MIN_SWITCH
    INIT_INPUT_PIN(Z_MIN_SWITCH);
#endif
#ifdef Z_MAX_SWITCH
    INIT_INPUT_PIN(Z_MAX_SWITCH);
#endif
}

#ifdef X_MIN_SWITCH
static inline bool x_min_reached(void)
{
    return REG_BIT_IS(X_MIN_SWITCH_PIN, X_MIN_SWITCH_REACHED);
}
#endif

#ifdef X_MAX_SWITCH
static inline bool x_max_reached(void)
{
    return REG_BIT_IS(X_MAX_SWITCH_PIN, X_MAX_SWITCH_REACHED);
}
#endif

#ifdef Y_MIN_SWITCH
static inline bool y_min_reached(void)
{
    return REG_BIT_IS(Y_MIN_SWITCH_PIN, Y_MIN_SWITCH_REACHED);
}
#endif

#ifdef Y_MAX_SWITCH
static inline bool y_max_reached(void)
{
    return REG_BIT_IS(Y_MAX_SWITCH_PIN, Y_MAX_SWITCH_REACHED);
}
#endif

#ifdef Z_MIN_SWITCH
static inline bool z_min_reached(void)
{
    return REG_BIT_IS(Z_MIN_SWITCH_PIN, Z_MIN_SWITCH_REACHED);
}
#endif

#ifdef Z_MAX_SWITCH
static inline bool z_max_reached(void)
{
    return REG_BIT_IS(Z_MAX_SWITCH_PIN, Z_MAX_SWITCH_REACHED);
}
#endif

#endif /* !LIMIT_SWITCHES_included */
