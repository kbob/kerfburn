#ifndef RELAYS_included
#define RELAYS_included

#include "config/pin-defs.h"
#include "pin-io.h"

// Interface

static inline void init_relays             (void);

static inline bool high_voltage_is_enabled (void);
static inline bool air_pump_is_enabled     (void);
static inline bool water_pump_is_enabled   (void);

static inline void enable_high_voltage     (void);
static inline void disable_high_voltage    (void);

static inline void enable_air_pump         (void);
static inline void disable_air_pump        (void);

static inline void enable_water_pump       (void);
static inline void disable_water_pump      (void);

// Implementation

static inline void init_relays(void)
{
    INIT_OUTPUT_PIN(HIGH_VOLTAGE_ENABLE, HIGH_VOLTAGE_DISABLED);
    INIT_OUTPUT_PIN(AIR_PUMP_ENABLE,     AIR_PUMP_DISABLED);
    INIT_OUTPUT_PIN(WATER_PUMP_ENABLE,   WATER_PUMP_DISABLED);
}

static inline bool high_voltage_is_enabled(void)
{
    return REG_BIT_IS(HIGH_VOLTAGE_ENABLE_PORT, HIGH_VOLTAGE_ENABLED);
}

static inline bool air_pump_is_enabled(void)
{
    return REG_BIT_IS(AIR_PUMP_ENABLE_PORT, AIR_PUMP_ENABLED);
}

static inline bool water_pump_is_enabled(void)
{
    return REG_BIT_IS(WATER_PUMP_ENABLE_PORT, WATER_PUMP_ENABLED);
}

static inline void enable_high_voltage(void)
{
    SET_REG_BIT(HIGH_VOLTAGE_ENABLE_PORT, HIGH_VOLTAGE_ENABLED);
}

static inline void disable_high_voltage(void)
{
    SET_REG_BIT(HIGH_VOLTAGE_ENABLE_PORT, HIGH_VOLTAGE_DISABLED);
}

static inline void enable_air_pump(void)
{
    SET_REG_BIT(AIR_PUMP_ENABLE_PORT, AIR_PUMP_ENABLED);
}

static inline void disable_air_pump(void)
{
    SET_REG_BIT(AIR_PUMP_ENABLE_PORT, AIR_PUMP_DISABLED);
}

static inline void enable_water_pump(void)
{
    SET_REG_BIT(WATER_PUMP_ENABLE_PORT, WATER_PUMP_ENABLED);
}

static inline void disable_water_pump(void)
{
    SET_REG_BIT(WATER_PUMP_ENABLE_PORT, WATER_PUMP_DISABLED);
}

#endif /* !RELAYS_included */
