#include "serial.h"

#include <util/atomic.h>

#include "fw_assert.h"

#define BAUD_RATE   9600
#define RX_BUF_SIZE 512
#define TX_BUF_SIZE 256

uint8_t rx_buf[RX_BUF_SIZE] __attribute__((aligned(256)));
uint16_t rx_head;
uint16_t rx_tail;
uint8_t  rx_errs;

uint8_t  tx_buf[TX_BUF_SIZE] __attribute__((aligned(256)));
uint8_t  tx_head;
uint8_t  tx_tail;
uint8_t  tx_errs;

void init_serial(void)
{
    const uint16_t baud_setting = F_CPU / 8 / BAUD_RATE - 1;

    // Enable double speed operation.
    UCSR0A = _BV(U2X0);

    // Set baud rate.
    UBRR0 = baud_setting;

    // Enable RX, TX, Data Register Empty Interrupt, and RX Complete Interrupt.
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(UDRIE0) | _BV(RXCIE0);
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

bool serial_rx_is_ready(void)
{
    bool ready;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ready = rx_errs || rx_head != rx_tail;
    }
    return ready;
}

uint16_t serial_rx_count(void)
{
    uint16_t count;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        count = (RX_BUF_SIZE + rx_tail - rx_head) % RX_BUF_SIZE;
    }    
    return count;
}

uint8_t serial_rx_peek_char(uint16_t pos)
{
    uint8_t c;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fw_assert(pos < serial_rx_count());
        c = rx_buf[(rx_head + pos) % RX_BUF_SIZE];
    }
    return c;
}

void serial_rx_consume(uint16_t count)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        fw_assert(count <= serial_rx_count());
        rx_head = (rx_head + count) % RX_BUF_SIZE;
    }
}

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
    bool avail;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        avail = tx_head != tx_tail + 1 && bit_is_set(UCSR0A, UDRE0);
    }
    return avail;
}

bool serial_tx_put_char(uint8_t c)
{
    bool ok = true;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (bit_is_set(UCSR0A, UDRE0))
            UDR0 = c;
        else if (tx_tail + 1 != tx_head)
            tx_buf[tx_tail++] = c;
        else {
            tx_errs |= SE_DATA_OVERRUN;
            ok = false;
        }            
    }
    return ok;
}

ISR(USART0_RX_vect)
{
    rx_errs |= UCSR0A & (_BV(UPE0) | _BV(DOR0) | _BV(FE0));
    if (bit_is_set(UCSR0A, RXC0)) {
        uint8_t c = UDR0;
        uint16_t new_tail = (rx_tail + 1) % RX_BUF_SIZE;
        if (new_tail == rx_head)
            rx_errs |= SE_DATA_OVERRUN;
        else {
            rx_buf[rx_tail] = c;
            rx_tail = new_tail;
        }
    }
}

ISR(USART0_UDRE_vect)
{
    if (tx_head != tx_tail)
        UDR0 = tx_buf[tx_head++];
}
