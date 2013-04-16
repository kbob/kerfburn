#ifndef LOW_VOLTAGE_included
#define LOW_VOLTAGE_included

#include "config/pin-defs.h"

#include "pin-io.h"

static inline void init_low_voltage_power(void)
{
    INIT_OUTPUT_PIN(LOW_VOLTAGE_ENABLE, LOW_VOLTAGE_DISABLED);
    INIT_INPUT_PIN(LOW_VOLTAGE_READY);
}

static inline bool low_voltage_is_enabled(void)
{
    return REG_BIT_IS(LOW_VOLTAGE_ENABLE_PIN, LOW_VOLTAGE_ENABLED);
}

static inline bool low_voltage_is_ready(void)
{
    return REG_BIT_IS(LOW_VOLTAGE_READY_PIN, LOW_VOLTAGE_READY);
}

static inline void enable_low_voltage(void)
{
    SET_REG_BIT(LOW_VOLTAGE_ENABLE_PORT, LOW_VOLTAGE_ENABLED);
}

static inline void disable_low_voltage(void)
{
    SET_REG_BIT(LOW_VOLTAGE_ENABLE_PORT, LOW_VOLTAGE_DISABLED);
}

#endif /* !LOW_VOLTAGE_included */
