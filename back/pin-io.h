#ifndef PIN_IO_included
#define PIN_IO_included

#include <avr/io.h>

#define LOW  0
#define HIGH 1

#define SET_BIT(reg, bit, value)                                \
    ((value) ? ((reg) |= _BV((bit))) : ((reg) &= ~_BV((bit))))

#define SET_REG_BIT(reg_bit, value)                             \
    SET_BIT(reg_bit##_reg, reg_bit##_bit, (value))

#define INIT_OUTPUT_PIN(pin, value)                             \
    (SET_REG_BIT(pin##_PORT, (value)),                          \
     pin##_DDR_reg |= _BV(pin##_DD_bit),                        \
     SET_REG_BIT(pin##_PORT, (value)))

#define INIT_INPUT_PIN(pin)                                     \
    (pin##_DDR_reg &= ~_BV(pin##_DD_bit),                       \
     SET_REG_BIT(pin##_PORT, LOW))

#define INIT_PULLUP_PIN(pin)                                    \
    (pin##_DDR_reg &= ~_BV(pin##_DD_bit),                       \
     SET_REG_BIT(pin##_PORT, HIGH))

#define REG_BIT_IS(reg_bit, value)                              \
    ((value) ?                                                  \
     bit_is_set(reg_bit##_reg, reg_bit##_bit) :                 \
     bit_is_clear(reg_bit##_reg, reg_bit##_bit))

#endif /* !PIN_IO_includaed */
