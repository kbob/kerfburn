#include "parser.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "actions.h"
#include "fault.h"
#include "fw_assert.h"
#include "serial.h"
#include "variables.h"

// Commands
// Da Dh Dl Dr Dw Dx Dy Dz
// Ea Eh El Er Ew Ex Ey Ez
// I
// Qc Qd Qe Qh Qm
// R
// S
// W

#define CMD_NOT_FOUND 0xFF      // returned by lookup_command()
#define CMD_NAME_SIZE    3      // max command name size, including NUL byte

typedef char             command_name[CMD_NAME_SIZE];
typedef void             command_function(void);
typedef command_name     c_name;
typedef command_function c_func;

typedef struct command_descriptor {
    PGM_P   cd_name;
    c_func *cd_func;
} command_descriptor, c_desc;

#define DEFINE_COMMAND_NAME(cmd) \
    static const char cmd##_name[] PROGMEM = #cmd

DEFINE_COMMAND_NAME(Da);
DEFINE_COMMAND_NAME(Dh);
DEFINE_COMMAND_NAME(Dl);
DEFINE_COMMAND_NAME(Dr);
DEFINE_COMMAND_NAME(Dw);
DEFINE_COMMAND_NAME(Dx);
DEFINE_COMMAND_NAME(Dy);
DEFINE_COMMAND_NAME(Dz);
DEFINE_COMMAND_NAME(Ea);
DEFINE_COMMAND_NAME(Eh);
DEFINE_COMMAND_NAME(El);
DEFINE_COMMAND_NAME(Er);
DEFINE_COMMAND_NAME(Ew);
DEFINE_COMMAND_NAME(Ex);
DEFINE_COMMAND_NAME(Ey);
DEFINE_COMMAND_NAME(Ez);
DEFINE_COMMAND_NAME(I);
DEFINE_COMMAND_NAME(Qc);
DEFINE_COMMAND_NAME(Qd);
DEFINE_COMMAND_NAME(Qe);
DEFINE_COMMAND_NAME(Qh);
DEFINE_COMMAND_NAME(Qm);
DEFINE_COMMAND_NAME(R);
DEFINE_COMMAND_NAME(S);
DEFINE_COMMAND_NAME(W);

static const c_desc command_descriptors[] PROGMEM = {
    { Da_name, action_disable_air_pump     },
    { Dh_name, action_disable_high_voltage },
    { Dl_name, action_disable_low_voltage  },
    { Dr_name, action_disable_reporting    },
    { Dw_name, action_disable_water_pump   },
    { Dx_name, action_disable_X_motor      },
    { Dy_name, action_disable_Y_motor      },
    { Dz_name, action_disable_Z_motor      },
    { Ea_name, action_enable_air_pump      },
    { Eh_name, action_enable_high_voltage  },
    { El_name, action_enable_low_voltage   },
    { Er_name, action_enable_reporting     },
    { Ew_name, action_enable_water_pump    },
    { Ex_name, action_enable_X_motor       },
    { Ey_name, action_enable_Y_motor       },
    { Ez_name, action_enable_Z_motor       },
    { I_name,  action_illuminate           },
    { Qc_name, action_enqueue_cut          },
    { Qd_name, action_enqueue_dwell        },
    { Qe_name, action_enqueue_engrave      },
    { Qh_name, action_enqueue_home         },
    { Qm_name, action_enqueue_move         },
    { R_name,  action_report_status        },
    { S_name,  action_stop                 },
    { W_name,  action_wait                 },
};

#define COMMAND_COUNT \
    (sizeof command_descriptors / sizeof command_descriptors[0])

static void get_cmd_name(uint8_t i, c_name *out)
{
    fw_assert(i < COMMAND_COUNT);
    PGM_P ptr = (PGM_P)pgm_read_word(&command_descriptors[i].cd_name);
    strncpy_P(*out, ptr, sizeof *out);
    (*out)[sizeof *out - 1] = '\0';
}

static c_func *get_cmd_func(uint8_t i)
{
    fw_assert(i < COMMAND_COUNT);
    return (c_func *)pgm_read_word(&command_descriptors[i].cd_func);
}

void init_parser(void)
{
#ifndef FW_NDEBUG
    c_name prev_name, curr_name;
    for (uint8_t i = 0; i < COMMAND_COUNT; i++) {
        get_cmd_name(i, &curr_name);
        if (i)
            fw_assert(strcmp(prev_name, curr_name) < 0);
        strncpy(prev_name, curr_name, sizeof prev_name);
    }
#endif
}


static uint8_t lookup_command(c_name name)
{
    uint8_t lo = 0, hi = COMMAND_COUNT;
    while (lo < hi) {
        uint8_t mid = (lo + hi) / 2;
        c_name mid_name;
        get_cmd_name(mid, &mid_name);
        int c = strcmp(name, mid_name);
        if (c == 0)
            return mid;
        if (c < 0)
            hi = mid;
        else
            lo = mid + 1;
    }
    return CMD_NOT_FOUND;
}

static inline bool is_eol(uint8_t c)
{
    return c == '\r' || c == '\n';
}

static inline bool is_digit(uint8_t c)
{
    return c >= '0' && c <= '9';
}

#define PARSE_ERROR() (parse_error(__LINE__))

static void parse_error(uint16_t line)
{
    // Send uninformative message, consume current line and continue.
    printf_P(PSTR("Parse error %u at \""), line);
    while (true) {
        while (!serial_rx_has_chars())
            continue;
        uint8_t e = serial_rx_errors();
        if (e)
            fw_assert(0 && "XXX Write me !");
        uint8_t c = serial_rx_peek_char(0);
        serial_rx_consume(1);
        if (is_eol(c))
            break;
        putchar(c);
    }
    printf_P(PSTR("\"\n"));
}

static bool consume_line(uint8_t pos)
{
    uint8_t c = serial_rx_peek_char(pos);
    if (!is_eol(c)) {
        PARSE_ERROR();
        return false;
    }
    serial_rx_consume(pos + 1);
    return true;
}

static inline void parse_action(uint8_t c0)
{
    c_name name;
    name[0] = c0;
    uint8_t pos = 1;
    uint8_t c1 = serial_rx_peek_char(1);
    if (!is_eol(c1))
        name[pos++] = c1;
    name[pos] = '\0';
    uint8_t index = lookup_command(name);
    if (index == CMD_NOT_FOUND) {
        PARSE_ERROR();
        return;
    }
    if (consume_line(pos)) {
        c_func *action = get_cmd_func(index);
        (*action)();
    }
}

static inline void parse_assignment(uint8_t c0)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (is_eol(c1)) {
        PARSE_ERROR();
        return;
    }
    uint8_t c2 = serial_rx_peek_char(2);
    if (c2 != '=') {
        PARSE_ERROR();
        return;
    }
    v_name name = { c0, c1, '\0' };
    v_index index = lookup_variable(name);
    if (index == VAR_NOT_FOUND) {
        PARSE_ERROR();
        return;
    }
    v_value value;
    uint8_t pos = 3;
    bool is_negative = false;
    switch (get_variable_type(index)) {

    case VT_SIGNED:
        {
            uint8_t c3 = serial_rx_peek_char(3);
            pos++;
            if (c3 == '-')
                is_negative = true;
            else if (c3 != '+') {
                PARSE_ERROR();
                return;
            }
        }
        // Fall through.

    case VT_UNSIGNED:
        {
            uint32_t n = 0;
            uint8_t c;
            while (is_digit((c = serial_rx_peek_char(pos)))) {
                n = 10 * n + (c - '0');
                pos++;
            }
            if (is_negative)
                value.vv_signed = -(int32_t)n;
            else
                value.vv_unsigned = n;
        }
        break;

    case VT_ENUM:
        {
            uint8_t c3 = serial_rx_peek_char(3);
            pos++;
            if (!variable_enum_is_OK(index, c3)) {
                PARSE_ERROR();
                return;
            }
            value.vv_enum = c3;
        }
        break;

    default:
        fw_assert(false);
        return;
    }
    if (consume_line(pos)) {
        // printf_P(PSTR("set %s = %"PRId32"\n"), name, value.vv_signed);
        set_variable(index, value);
        if (name[0] == 'o')
            update_overrides();
    }
}

void parse_line(void)
{
    uint8_t c0 = serial_rx_peek_char(0);
    if (is_eol(c0)) {
        serial_rx_consume(1);
        return;
    }

    const uint8_t hi_mask = 0xE0;
    uint8_t c0_hi_bits = c0 & hi_mask;
    if (c0_hi_bits == ('A' & hi_mask))
        parse_action(c0);
    else if (c0_hi_bits == ('a' & hi_mask))
        parse_assignment(c0);
    else
        PARSE_ERROR();
}
