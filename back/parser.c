#include "parser.h"

#include <stdint.h>

#include "actions.h"
#include "fw_assert.h"
#include "param.h"
#include "serial.h"

#if 0
Params
dt ia il lm lp ls pd pi pl x0 xa xd y0 ya yd

Commands
Da Dh Dl Dw Dx Dy Dz
El Eh Ea Ew Ex Ey Ez
H
I
Qc Qd Qe Qh Qm
S? S? S?
W
#endif

static void error(void)
{
    fw_assert(false);
    // XXX Send message, consume current line and continue.
}

static inline bool is_eol(uint8_t c)
{
    return c == '\r' || c == '\n';
}

static inline bool is_digit(uint8_t c)
{
    return c >= '0' && c <= '9';
}

static bool check_at_eol(uint8_t pos)
{
    uint8_t c = serial_rx_peek_char(pos);
    if (!is_eol(c)) {
        error();
        return false;
    }
    serial_rx_consume(pos + 1);
    return true;
}

static inline void parse_enable_action(void)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (!check_at_eol(2)) {
        error();
        return;
    }
    switch (c1) {

    case 'a':
        action_enable_air_pump();
        break;

    case 'h':
        action_enable_high_voltage();
        break;

    case 'l':
        action_enable_low_voltage();
        break;

    case 'w':
        action_enable_water_pump();
        break;

    case 'x':
        action_enable_X_motor();
        break;

    case 'y':
        action_enable_Y_motor();
        break;

    case 'z':
        action_enable_Z_motor();
        break;

    default:
        error();
    }
}

static inline void parse_disable_action(void)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (!check_at_eol(2)) {
        error();
        return;
    }
    switch (c1) {

    case 'a':
        action_disable_air_pump();
        break;

    case 'h':
        action_disable_high_voltage();
        break;

    case 'l':
        action_disable_low_voltage();
        break;

    case 'w':
        action_disable_water_pump();
        break;

    case 'x':
        action_disable_X_motor();
        break;

    case 'y':
        action_disable_Y_motor();
        break;

    case 'z':
        action_disable_Z_motor();
        break;

    default:
        error();
    }
}

static inline void parse_enqueue_action(void)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (!check_at_eol(2)) {
        error();
        return;
    }
    switch (c1) {

    case 'd':
        action_enqueue_dwell();
        break;

    case 'm':
        action_enqueue_move();
        break;

    case 'c':
        action_enqueue_cut();
        break;

    case 'e':
        action_enqueue_engrave();
        break;

    case 'h':
        action_enqueue_home();
        break;

    default:
        error();
        break;
    }
}

static inline void parse_status_action(void)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (!check_at_eol(2)) {
        error();
        return;
    }
    switch (c1) {

    case 'f':
        action_send_foo_status();
        break;

    case 'b':
        action_send_bar_status();
        break;

    case 'z':
        action_send_baz_status();
        break;

    default:
        error();
        break;
    }
}

static inline void parse_action(uint8_t c0)
{
    switch (c0) {

    case 'D':
        parse_disable_action();
        break;

    case 'E':
        parse_enable_action();
        break;

    case 'H':
        if (check_at_eol(1))
            action_emergency_stop();
        break;

    case 'I':
        if (check_at_eol(1))
            action_illuminate();
        break;

    case 'Q':
        parse_enqueue_action();
        break;

    case 'S':
        parse_status_action();
        break;

    case 'W':
        if (check_at_eol(1))
            action_wait();
        break;

    default:
        error();
        break;
    }
}

static inline void parse_assignment(uint8_t c0)
{
    uint8_t c1 = serial_rx_peek_char(1);
    if (is_eol(c1)) {
        error();
        return;
    }
    uint8_t c2 = serial_rx_peek_char(2);
    if (c2 != '=') {
        error();
        return;
    }
    char vname[3] = { c0, c2, '\0' };;
    uint8_t index = lookup_param(vname);
    if (index == 0xFF) {
        error();
        return;
    }
    const param_descriptor *desc = &param_descriptors[index];
    uint8_t type = desc->p_type;
    param_value value;
    uint8_t pos = 3;
    bool is_negative = false;
    switch (type) {

    case PT_SIGNED:
        {
            uint8_t c3 = serial_rx_peek_char(3);
            pos++;
            if (c3 == '-')
                is_negative = true;
            else if (c3 != '+') {
                error();
                return;
            }
        }
        // Fall through.

    case PT_UNSIGNED:
        {
            uint32_t n = 0;
            uint8_t c;
            while (is_digit((c = serial_rx_peek_char(pos++))))
                n = 10 * n + (c - '0');
            if (is_negative)
                value.pv_signed = -(int32_t)n;
            else
                value.pv_unsigned = n;
        }
        break;

    case PT_ENUM:
        {
            uint8_t c3 = serial_rx_peek_char(3);
            pos++;
            if (!param_enum_is_OK(index, c3)) {
                error();
                return;
            }
            value.pv_enum = c3;
        }
        break;

    default:
        fw_assert(false);
    }
    if (check_at_eol(pos))
        assign_param(index, value);
}

void parse(void)
{
    uint8_t c0 = serial_rx_peek_char(0);
    if (is_eol(c0)) {
        serial_rx_consume(1);
        return;
    }

    const uint8_t hi_mask = 0xC0;
    uint8_t c0_hi_bits = c0 & hi_mask;
    if (c0_hi_bits == ('A' & hi_mask))
        parse_action(c0);
    else if (c0_hi_bits == ('a' & hi_mask))
        parse_assignment(c0);
    else
        error();
}
