#ifndef PIN_IO_included
#define PIN_IO_included

#include <avr/io.h>

#define LOW  0
#define HIGH 1

// Test a bit in a register, as defined in pin-mappings.py.
//
// E.g., if pin_mappings defined an input pin like this,
//
//     def_input_pin(PA2, 'Frob State', enabled=low)
//
// then REG_BIT_IS(FROB_STATE_PIN, FROB_STATE_DISABLED) would compile into
// code equivalent to this.
//
//     ((PINA & (1 << 2)) == HIGH)
//
// (N.B., gen_pin_mappings defines some antonym symbols: enabled !=
// disabled, on != off, etc.)

#define REG_BIT_IS(reg_bit, value)                              \
    ((value) ?                                                  \
     bit_is_set(reg_bit##_reg, reg_bit##_bit) :                 \
     bit_is_clear(reg_bit##_reg, reg_bit##_bit))


// Set a bit in a register, as defined in pin-mappings.py.
//
// E.g., if pin_mappings defined an output pin like this,
//
//     def_output_pin(PB5, 'Foo Bar', snazzy=low, crummy=high)
//
// then SET_REG_BIT(FOO_BAR, FOO_BAR_CRUMMY) would compile into code
// equivalent to this.
//
//     (PORTB |= 1 << 5)

#define SET_REG_BIT(reg_bit, value)                             \
    SET_BIT(reg_bit##_reg, reg_bit##_bit, (value))

#define SET_BIT(reg, bit, value)                                \
    ((value) ? ((reg) |= _BV((bit))) : ((reg) &= ~_BV((bit))))


// Initialize an input or output pin as defined in pin-mappings.py.

#define INIT_OUTPUT_PIN(pin, value)                             \
    (SET_REG_BIT(pin##_PORT, (value)),                          \
     pin##_DDR_reg |= _BV(pin##_DD_bit),                        \
     SET_REG_BIT(pin##_PORT, (value)))

#define INIT_INPUT_PIN(pin)                                     \
    (pin##_DDR_reg &= ~_BV(pin##_DD_bit),                       \
     SET_REG_BIT(pin##_PORT, pin##_pullup))

#endif /* !PIN_IO_included */
