#ifndef PIN_IO_included
#define PIN_IO_included

#include <avr/io.h>

#define LOW  0
#define HIGH 1

#define SET_BIT(reg, bit, value)                                \
    ((value) ? ((reg) |= _BV((bit))) : ((reg) &= ~_BV((bit))))

#define SET_PIN_BIT(pin_bit, value)                             \
    SET_BIT(pin_bit##_reg, pin_bit##_bit, (value))

#define INIT_OUTPUT_PIN(pin, value)                             \
    (SET_PIN_BIT(pin##_PORT, (value)),                          \
     pin##_DDR_reg |= _BV(pin##_DD_bit),                        \
     SET_PIN_BIT(pin##_PORT, (value)))

#define INIT_INPUT_PIN(pin)                                     \
    (pin##_DDR_reg &= ~_BV(pin##_DD_bit),                       \
     SET_PIN_BIT(pin##_PORT, LOW))

#define INIT_PULLUP_PIN(pin)                                    \
    (pin##_DDR_reg &= ~_BV(pin##_DD_bit),                       \
     SET_PIN_BIT(pin##_PORT, HIGH))

#define PIN_BIT_IS(pin_bit, value)                              \
    ((value) ?                                                  \
     bit_is_set(pin_bit##_reg, pin_bit##_bit) :                 \
     bit_is_clear(pin_bit##_reg, pin_bit##_bit))

#endif /* !PIN_IO_includaed */
