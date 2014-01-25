#include "report.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include <avr/pgmspace.h>

#include "e-stop.h"
#include "fault.h"
#include "fw_stdio.h"
#include "limit-switches.h"
#include "low-voltage.h"
#include "memory.h"
#include "motors.h"
#include "queues.h"
#include "relays.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"

typedef void report_func(void);

typedef struct report_descriptor {
    uint8_t      rd_var;
    report_func *rd_func;
} report_descriptor, r_desc;

static          timeout report_timeout;
static volatile bool    reporting_is_active;

#define DEFINE_UNIMPLEMENTED_REPORT(code, name)                         \
    static void report_##name(void)                                     \
    {                                                                   \
        printf_P(PSL(#code " (" #name ") report not implemented\n"));   \
    }

static void report_e_stop(void)
{
    printf_P(PSL("E e=%c\n"), is_emergency_stopped() ? 'y' : 'n');
}

static void report_faults(void)
{
    putchar('F');
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (fault_is_set(i)) {
            putchar(' ');
            if (fault_is_overridden(i))
                putchar('!');
            f_name name;
            get_fault_name(i, &name);
            printf_P(PSL("%s"), name);
        }
    }
    putchar('\n');
}

static void report_limit_switches(void)
{
#ifdef X_MIN_SWITCH
    const char xmin = x_min_reached() ? 'y' : 'n';
#else
    const char xmin = '_';
#endif

#ifdef X_MAX_SWITCH
    const char xmax = x_max_reached() ? 'y' : 'n';
#else
    const char xmax = '_';
#endif

#ifdef Y_MIN_SWITCH
    const char ymin = y_min_reached() ? 'y' : 'n';
#else
    const char ymin = '_';
#endif

#ifdef Y_MAX_SWITCH
    const char ymax = y_max_reached() ? 'y' : 'n';
#else
    const char ymax = '_';
#endif

#ifdef Z_MIN_SWITCH
    const char zmin = z_min_reached() ? 'y' : 'n';
#else
    const char zmin = '_';
#endif

#ifdef Z_MAX_SWITCH
    const char zmax = z_max_reached() ? 'y' : 'n';
#else
    const char zmax = '_';
#endif

    printf_P(PSL("L x=%c%c y=%c%c z=%c%c\n"),
           xmin, xmax, ymin, ymax, zmin, zmax);
}

static void report_motors(void)
{
    char xe = x_step_is_enabled()       ? 'e' : 'd';
    char xd = x_direction_is_positive() ? '+' : '-';
    char ye = y_step_is_enabled()       ? 'e' : 'd';
    char yd = y_direction_is_positive() ? '+' : '-';
    char ze = z_step_is_enabled()       ? 'e' : 'd';
    char zd = z_direction_is_positive() ? '+' : '-';

    printf_P(PSL("M x=%c%c y=%c%c z=%c%c\n"), xe, xd, ye, yd, ze, zd);
}

static void report_power(void)
{
    char low_voltage_enabled = low_voltage_is_enabled()  ? 'y' : 'n';
    char low_voltage_ready   = low_voltage_is_ready()    ? 'y' : 'n';
    char high_voltage        = high_voltage_is_enabled() ? 'y' : 'n';
    char air                 = air_pump_is_enabled()     ? 'y' : 'n';
    char water               = water_pump_is_enabled()   ? 'y' : 'n';
    printf_P(PSL("P le=%c lr=%c he=%c ae=%c we=%c\n"),
           low_voltage_enabled, low_voltage_ready, high_voltage, air, water);
}

static void report_queues(void)
{
    printf_P(PSL("Q x=%u y=%u z=%u p=%u\n"),
             queue_length(&Xq), queue_length(&Yq),
             queue_length(&Zq), queue_length(&Pq));
}

static void report_RAM(void)
{
    // Technically, text is in program memory, not RAM.
    segment_sizes ss;
    get_memory_use(&ss);
    printf_P(PSL("R t=%u d=%u b=%u f=%u s=%u\n"),
             ss.ss_text, ss.ss_data, ss.ss_bss, ss.ss_free, ss.ss_stack);
}

static void report_serial(void)
{
    uint8_t rc = serial_rx_char_count();
    uint8_t rl = serial_rx_line_count();
    uint8_t re = serial_rx_peek_errors();
    uint8_t tc = serial_tx_char_count();
    uint8_t te = serial_tx_peek_errors();
    printf_P(PSL("S rx c=%"PRId8" r=%"PRId8" e=%#"PRIx8", "
                 "tx c=%"PRId8" e=%#"PRIx8"\n"),
             rc, rl, re, tc, te);
}

static void report_variables(void)
{
    putchar('V');
    for (uint8_t i = 0; i < VARIABLE_COUNT; i++) {
        v_name name;
        get_variable_name(i, &name);
        v_type type = get_variable_type(i);
        v_value value = get_variable(i);

        printf_P(PSL(" %s="), name);
        switch (type) {
        case VT_UNSIGNED:
            printf_P(PSL("%"PRIu32), value.vv_unsigned);
            break;

        case VT_SIGNED:
            printf_P(PSL("%+"PRId32), value.vv_signed);

            break;
        case VT_ENUM:
            printf_P(PSL("%c"), (int)value.vv_enum);
            break;

        default:
            fw_assert(false);
        }
    }
    putchar('\n');
}

DEFINE_UNIMPLEMENTED_REPORT(W, water);

static const report_descriptor report_descriptors[] PROGMEM = {
    { V_RE, report_e_stop         },
    { V_RF, report_faults         },
    { V_RL, report_limit_switches },
    { V_RM, report_motors         },
    { V_RP, report_power          },
    { V_RQ, report_queues         },
    { V_RR, report_RAM            },
    { V_RS, report_serial         },
    { V_RV, report_variables      },
    { V_RW, report_water          },
};

static const size_t report_descriptor_count =
    sizeof report_descriptors / sizeof report_descriptors[0];

void init_reporting(void)
{
    // ??? Anything to do?
}

void report_all(void)
{
    if (reporting_is_active)
        return;
    reporting_is_active = true;
    for (uint8_t i = 0; i < report_descriptor_count; i++) {
        const r_desc *rdp = report_descriptors + i;
        const uint8_t var = pgm_read_byte(&rdp->rd_var);
        if (get_enum_variable(var) == 'y') {
            report_func *func = (report_func *)pgm_read_word(&rdp->rd_func);
            (*func)();
        }
    }
    reporting_is_active = false;
}

extern void enable_reporting(void)
{
    uint32_t interval = get_unsigned_variable(V_RI);
    fw_assert(interval >= 10);
    report_timeout.to_interval = interval;
    report_timeout.to_func = report_all;
    enqueue_timeout(&report_timeout, millisecond_time() + interval);
}

extern void disable_reporting(void)
{
    dequeue_timeout(&report_timeout);
}
