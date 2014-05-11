#include <stdbool.h>
#include <stdio.h>

// #include "e-stop.h"
#include "engine.h"
#include "fault.h"
#include "fw_stdio.h"
#include "illum.h"
#include "lasers.h"
#include "LEDs.h"
#include "limit-switches.h"
#include "low-voltage.h"
#include "memory.h"
#include "motors.h"
#include "parser.h"
#include "queues.h"
#include "relays.h"
#include "report.h"
#include "safety.h"
#include "scheduler.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"
#include "version.h"

static void initialize_devices(void)
{
    // Initialize hardware modules.
    init_timer();
    init_serial();
    init_stdio();
    init_safety();
    // init_emergency_stop();
    // init_lid();
    init_limit_switches();
    init_low_voltage_power();
    init_relays();
    init_motors();
    init_lasers();
    init_LEDs();
    init_illumination();

    // Hardware ready -- enable interrupts.
    sei();

    // Initialize software modules.
    init_variables();
    init_reporting();
    init_parser();
    init_atoms();
    init_queues();
    init_engine();
    init_scheduler();
}

static void trigger_serial_faults(uint8_t e)
{
    if (e & SE_FRAME_ERROR)
        trigger_fault(F_SF);
    if (e & SE_DATA_OVERRUN)
        trigger_fault(F_SO);
    if (e & SE_PARITY_ERROR)
        trigger_fault(F_SP);
}

static void do_background_task(void)
{
    serial_rx_start();
    printf_P(version);
    printf_P(PSTR("\nReady\n"));
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
    init_memory_monitor();      // Do this first.
    initialize_devices();
    while (true)
        do_background_task();
}
