#include <stdbool.h>
#include <stdio.h>

#include "fault.h"
#include "fw_stdio.h"
#include "illum.h"
#include "LEDs.h"
#include "low-voltage.h"
#include "motors.h"
#include "parser.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"
#include "version.h"

// Test these.
// DONE millisecond time updates.
// DONE softint triggered from background
//      softint triggered from softint
//      softint triggered from softint

static void initialize_devices(void)
{
    init_timer();
    init_serial();
    init_stdio();
    init_low_voltage_power();
    init_motors();
    init_LEDs();
    init_illumination();
    // XXX more devices coming...
    sei();

    init_variables();
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
    printf_P(version);
    printf("\nReady\n");
    while (true) {
        while (!serial_rx_has_lines())
            continue;
        uint8_t e = serial_rx_errors();
        if (e) {
            trigger_serial_faults(e);
            continue;
        }
        parse_line();
    }
}

int main()
{
    initialize_devices();
    while (true)
        do_background_task();
}
