#include "serial.h"

#include <util/atomic.h>

#include "bufs.h"
#include "fault.h"
#include "fw_assert.h"

//#define BAUD_RATE   9600
#define BAUD_RATE 115200

#define TX_BUF_SIZE  256

#define RX_BUF_SIZE  256
#define RX_FLOW_SHIFT  4

#define ASCII_CAN  '\003'
#define ASCII_LF     '\n'
#define ASCII_CR     '\r'
#define ASCII_XON  '\021'
#define ASCII_XOFF '\023'

// tx_buf and rx_buf are actually defined in bufs.c.
//static uint8_t tx_buf[TX_BUF_SIZE] __attribute__((aligned(256)));
static uint8_t tx_head;
static uint8_t tx_tail;
static uint8_t tx_errs;
static uint8_t tx_oob_char;

//static uint8_t rx_buf[RX_BUF_SIZE] __attribute__((aligned(256)));
static uint8_t rx_head;
static uint8_t rx_tail;
static uint8_t rx_errs;
static uint8_t rx_line_count;

void init_serial(void)
{
    const uint16_t baud_setting = F_CPU / 8 / BAUD_RATE - 1;

    // Enable double speed operation.
    UCSR0A = _BV(U2X0);

    // Set baud rate.
    UBRR0 = baud_setting;

    // Enable RX, TX, and RX Complete Interrupt.
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
}

static inline bool is_eol_char(uint8_t c)
{
    return c == ASCII_CR || c == ASCII_LF;
}

// //  // //   // //  // //    // //  // //   // //  // //     // //  // //

uint8_t serial_tx_errors(void)
{
    uint8_t errs = tx_errs;
    tx_errs = 0;
    return errs;
}

uint8_t serial_tx_peek_errors(void)
{
    return tx_errs;
}

bool serial_tx_is_idle(void)
{
    bool idle;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        idle = tx_head == tx_tail && bit_is_set(UCSR0A, TXC0);
    }
    return idle;
}

uint8_t serial_tx_is_available(void)
{
    uint8_t h, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        h = tx_head;
        t = tx_tail;
    }
    return h != (t + 1) % TX_BUF_SIZE;
}

uint8_t serial_tx_char_count(void)
{
    uint8_t h, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        h = tx_head;
        t = tx_tail;
    }
    return (h - t + 1) % TX_BUF_SIZE;
}

bool serial_tx_put_char(uint8_t c)
{
    bool ok = true;
    uint8_t new_tail;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (bit_is_set(UCSR0A, UDRE0))
            UDR0 = c;
        else if ((new_tail = (tx_tail + 1) % TX_BUF_SIZE) != tx_head) {
            tx_buf[tx_tail] = c;
            tx_tail = new_tail;
            UCSR0B |= _BV(UDRIE0);
        } else {
            tx_errs |= SE_DATA_OVERRUN;
            ok = false;
        }
    }
    return ok;
}

static inline void tx_send_oob_NONATOMIC(uint8_t c)
{
    if (bit_is_set(UCSR0A, UDRE0)) {
        UDR0 = c;
    } else {
        tx_oob_char = c;
        UCSR0B |= _BV(UDRIE0);
    }
}

ISR(USART0_UDRE_vect)
{
    if (tx_oob_char) {
        UDR0 = tx_oob_char;
        tx_oob_char = '\0';
    } else if (tx_head != tx_tail) {
        UDR0 = tx_buf[tx_head];
        tx_head = (tx_head + 1) % TX_BUF_SIZE;
    } else
        UCSR0B &= ~_BV(UDRIE0);
}

// //  // //   // //  // //    // //  // //   // //  // //     // //  // //

void serial_rx_start(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rx_head = rx_tail = 0;
        tx_send_oob_NONATOMIC(-(1 << RX_FLOW_SHIFT));
    }
}

uint8_t serial_rx_errors(void)
{
    uint8_t errs = rx_errs;
    rx_errs = 0;
    return errs;
}

uint8_t serial_rx_peek_errors(void)
{
    return rx_errs;
}

bool serial_rx_has_chars(void)
{
    bool ready;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ready = rx_errs || rx_head != rx_tail;
    }
    return ready;
}

bool serial_rx_has_lines(void)
{
    bool ready;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ready = rx_errs || rx_line_count;
    }
    return ready;
}

uint8_t serial_rx_char_count(void)
{
    uint8_t h, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        h = rx_head;
        t = rx_tail;
    }
    return t - h;
}

uint8_t serial_rx_line_count(void)
{
    return rx_line_count;
}

uint8_t serial_rx_peek_char(uint8_t pos)
{
    uint8_t c;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fw_assert(pos < serial_rx_char_count());
        c = rx_buf[(rx_head + pos) % RX_BUF_SIZE];
    }
    return c;
}

void serial_rx_consume(uint8_t count)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fw_assert(count <= serial_rx_char_count());
        for (uint8_t i = 0; i < count; i++) {
            uint8_t c = rx_buf[rx_head];
            if (is_eol_char(c))
                --rx_line_count;
            rx_head++;
            if (!(rx_head & ~-(1 << RX_FLOW_SHIFT))) {
                uint8_t ack = rx_head >> RX_FLOW_SHIFT | -(1 << RX_FLOW_SHIFT);
                tx_send_oob_NONATOMIC(ack);
            }
        }
    }
}

ISR(USART0_RX_vect)
{
    rx_errs |= UCSR0A & (_BV(UPE0) | _BV(DOR0) | _BV(FE0));
    if (bit_is_set(UCSR0A, RXC0)) {
        uint8_t c = UDR0;
        if (c == ASCII_CAN)
            trigger_fault(F_ES);
        else {
            uint8_t new_tail = rx_tail + 1;
            if (new_tail == rx_head)
                rx_errs |= SE_DATA_OVERRUN;
            else {
                rx_buf[rx_tail] = c;
                rx_tail = new_tail;
                if (is_eol_char(c))
                    rx_line_count++;
            }
        }
    }
}
