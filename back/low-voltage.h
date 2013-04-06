#ifndef LOW_VOLTAGE_INCLUDED
#define LOW_VOLTAGE_INCLUDED

#include <stdbool.h>

#include "config/pin-defs.h"
#include "pin-io.h"

#if 0
#define LOW_VOLTAGE_ENABLE_DDR_reg  DDRB
#define LOW_VOLTAGE_ENABLE_DD_bit   DDB6
#define LOW_VOLTAGE_ENABLE_PIN_reg  PINB
#define LOW_VOLTAGE_ENABLE_PIN_bit  PINB6
#define LOW_VOLTAGE_ENABLE_PORT_reg PORTB
#define LOW_VOLTAGE_ENABLE_PORT_bit PORTB6

#define LOW_VOLTAGE_READY_DDR_reg   DDRA
#define LOW_VOLTAGE_READY_DD_bit    DDA0
#define LOW_VOLTAGE_READY_PIN_reg   PINA
#define LOW_VOLTAGE_READY_PIN_bit   PINA0
#define LOW_VOLTAGE_READY_PORT_reg  PORTA
#define LOW_VOLTAGE_READY_PORT_bit  PORTA0

#define LOW_VOLTAGE_ENABLED         LOW
#define LOW_VOLTAGE_DISABLED      (!LOW_VOLTAGE_ENABLED)

#define LOW_VOLTAGE_READY           HIGH
#endif

static inline void init_low_voltage_power(void)
{
    INIT_OUTPUT_PIN(LOW_VOLTAGE_ENABLE, LOW_VOLTAGE_ENABLE_DISABLED);
    INIT_INPUT_PIN(LOW_VOLTAGE_READY);
}

static inline bool low_voltage_is_enabled(void)
{
    return PIN_BIT_IS(LOW_VOLTAGE_ENABLE_PORT, LOW_VOLTAGE_ENABLE_ENABLED);
}

static inline bool low_voltage_is_ready(void)
{
    return PIN_BIT_IS(LOW_VOLTAGE_READY_PORT, LOW_VOLTAGE_READY_READY);
}

static inline void enable_low_voltage(void)
{
    SET_PIN_BIT(LOW_VOLTAGE_ENABLE_PORT, LOW_VOLTAGE_ENABLE_ENABLED);
}

static inline void disable_low_voltage(void)
{
    SET_PIN_BIT(LOW_VOLTAGE_ENABLE_PORT, LOW_VOLTAGE_ENABLE_DISABLED);
}

#endif /* !LOW_VOLTAGE_INCLUDED */
