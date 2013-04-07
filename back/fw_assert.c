#include "fw_assert.h"

// Firmware Assert.
//
// Loop forever:
//    print "Assertion failed on line %d.\r\n"
//    throb LED once.
//
// Interrupts are permanently off.
// Use local serial and LED drivers -- this will keep working when
// other drivers are broken.  (This will also break when other parts
// are working.
//
// If FW_NDEBUG is defined, this whole file goes away.

#ifndef FW_NDEBUG

#include <stdbool.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include "config/pin-defs.h"
#include "pin-io.h"

#define BAUD_RATE   9600

static inline void init_serial(void)
{
#if 0
    // Keep mode, baud rate, parity settings.
    UCSR0B &= ~_BV(RXEN0);
    UCSR0B &= ~_BV(TXEN0);
    UCSR0B |=  _BV(TXEN0);;
#else
    const uint16_t baud_setting = F_CPU / 8 / BAUD_RATE - 1;

    // Enable double speed operation.
    UCSR0A = _BV(U2X0);

    // Set baud rate.
    UBRR0 = baud_setting;

    // Enable RX, TX, Data Register Empty Interrupt, and RX Complete Interrupt.
    UCSR0B = _BV(TXEN0);

#endif
}

static inline void put_char(uint8_t c)
{
    while (bit_is_clear(UCSR0A, UDRE0))
        continue;
    UDR0 = c;
}

static inline void put_str(char *p)
{
    while (*p)
        put_char(*p++);
}

static inline void put_dec(unsigned int n)
{
    char b[5];
    uint8_t i = 0;
    do {
        b[i++] = '0' + n % 10;
    } while (n /= 10);
    while (i)
        put_char(b[--i]);
}

static inline void init_LED(void)
{
    INIT_OUTPUT_PIN(LED, LED_OFF);
}

static inline void light_LED(void)
{
    SET_REG_BIT(LED_PORT, LED_ON);
}

static inline void extinguish_LED(void)
{
    SET_REG_BIT(LED_PORT, LED_OFF);
}

static void throb_once(void)
{
    int duty;
    for (duty = 0; duty < 30; duty++) {
        light_LED();
        _delay_ms(duty);
        extinguish_LED();
        _delay_ms(30 - duty);
    }
    for (duty = 30; duty > 0; --duty) {
        light_LED();
        _delay_ms(duty);
        extinguish_LED();
        _delay_ms(30 - duty);
    }
}

__attribute__((noreturn))
extern void fw_assertion_failed(unsigned int line_no)
{
    cli();
    init_serial();
    init_LED();
    while (true) {
        put_str("Assertion failed on line ");
        put_dec(line_no);
        put_str(".\r\n");
        throb_once();
    }
}

#endif
