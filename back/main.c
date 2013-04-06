#include <stdbool.h>

#include "fault.h"
#include "illum.h"
#include "LEDs.h"
#include "low-voltage.h"
#include "parser.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"

// Test these.
// DONE millisecond time updates.
// DONE softint triggered from background
//      softint triggered from softint
//      softint triggered from softint

static void initialize_devices(void)
{
    init_variables();
    init_timer();
    init_serial();
    init_low_voltage_power();
    init_LEDs();
    init_illumination();
    // XXX more devices coming...
    sei();
}

static void trigger_serial_faults(uint8_t e)
{
    if (e & SE_FRAME_ERROR)
        trigger_fault(F_SERIAL_FRAME);
    if (e & SE_DATA_OVERRUN)
        trigger_fault(F_SERIAL_OVERRUN);
    if (e & SE_PARITY_ERROR)
        trigger_fault(F_SERIAL_PARITY);
}

static void do_background_task(void)
{
    
    serial_rx_start();
    while (true) {
        while (!serial_rx_has_lines())
            continue;
        uint8_t e = serial_rx_errors();
        if (e)
            trigger_serial_faults(e);
        parse_line();
    }
}

int main()
{
    initialize_devices();
    while (true)
        do_background_task();
}
