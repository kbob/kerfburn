#include "i2c.h"

#include <avr/interrupt.h>
#include <util/twi.h>

#include "config/pin-defs.h"

#include "fw_assert.h"
#include "pin-io.h"

#define I2C_BIT_RATE 100000
#define I2C_MAX 3

typedef enum i2c_state {
    IS_UNINIT,
    IS_IDLE,
    IS_MTX_BUSY,
} i2c_state;

static i2c_state state = IS_UNINIT;

static uint8_t  *tx_data;
static uint8_t   tx_count;
static uint8_t   tx_status;
static uint8_t   buf[I2C_MAX + 1];


#define OR3(a,b,c)     (_BV(a) | _BV(b) | _BV(c))
#define OR4(a,b,c,d)   (_BV(a) | _BV(b) | _BV(c) | _BV(d))
#define OR5(a,b,c,d,e) (_BV(a) | _BV(b) | _BV(c) | _BV(d) | _BV(e))


// Constants for TWCR.
#define TWC_INIT  (OR3(       TWEA,               TWEN, TWIE))
#define TWC_START (OR5(TWINT, TWEA, TWSTA,        TWEN, TWIE))
#define TWC_CONT  (OR4(TWINT, TWEA,               TWEN, TWIE))
#define TWC_STOP  (OR5(TWINT, TWEA,        TWSTO, TWEN, TWIE))
#define TWC_ABORT (OR4(TWINT, TWEA,               TWEN, TWIE))

void init_i2c(void)
{
    // enable pull-ups on SCL and SDA.
    INIT_INPUT_PIN(I2C_CLOCK);
    INIT_INPUT_PIN(I2C_DATA);

    // init two-wire interface
    TWSR = 0;
    TWBR = (F_CPU / I2C_BIT_RATE - 16) / 2;
    TWCR = TWC_INIT;
    state = IS_IDLE;
}

void i2cm_transmit(uint8_t slave_addr, uint8_t *data, uint8_t  size)
{
    fw_assert(size <= I2C_MAX);

    (void)i2cm_status();    // wait for previous transaction to finish
    buf[0] = slave_addr << 1 | TW_WRITE;
    for (uint8_t i = 0; i < size; i++)
        buf[i + 1] = data[i];
    tx_data = buf;
    tx_count = size + 1;
    TWCR = TWC_START;
}

uint8_t i2cm_status (void)
{
    while (state != IS_IDLE)
        continue;
    loop_until_bit_is_clear(TWCR, TWSTO);
    return tx_status;
}

ISR(TWI_vect)
{
    uint8_t tw_sts = TW_STATUS;
    switch (tw_sts) {

    case TW_START:
    case TW_MT_SLA_ACK:
    case TW_MT_DATA_ACK:
        if (tx_count) {
            // Transmit next byte.
            TWDR = *tx_data;
            TWCR = TWC_CONT;
            tx_data++;
            tx_count--;
        } else {
            // Done.  Transmit STOP.
            TWCR = TWC_STOP;
            tx_status = 0;
            state = IS_IDLE;
        }
        break;

    case TW_MT_SLA_NACK:
    case TW_MT_DATA_NACK:
    case TW_MT_ARB_LOST:
    default:
        tx_status = tw_sts;
        TWCR = TWC_ABORT;
        break;
    }
}
