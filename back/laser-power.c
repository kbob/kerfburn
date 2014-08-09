#include "laser-power.h"

//#include <avr/interrupt.h>

#include "i2c.h"

#define MCP4725_ADDR0         0x62
#define MCP4725_CMD_WRITE_DAC 0x40

void init_laser_power(void)
{
    set_laser_power(0);
}

void set_laser_power(uint16_t level)
{
    uint8_t buf[3] = { MCP4725_CMD_WRITE_DAC, level >> 4, level << 4 };
    i2cm_transmit(MCP4725_ADDR0, buf, 3);
    (void)i2cm_status();
}
