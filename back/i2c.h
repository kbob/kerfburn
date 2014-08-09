#ifndef I2C_included
#define I2C_included

#include <stdint.h>

extern void init_i2c(void);

// i2cm - i2c master
extern void    i2cm_transmit (uint8_t slave_addr, uint8_t *data, uint8_t size);
extern uint8_t i2cm_status   (void);

// i2cs - i2c slave
// not needed

#endif /* !I2C_include */
