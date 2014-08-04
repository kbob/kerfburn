#ifndef SAFETY_included
#define SAFETY_included

// Safety switches: emergency stop and lid open.

// Emergency Stop is triggered when:
//   + User presses the Big Red Button
//   + software raises an F_ES fault
//       - front end sends control-C
//
// Emergency Stop is cleared when:
//   + User releases the Big Red Button


// Interface

#include <stdbool.h>

extern void init_safety(void);

extern void clear_emergency(void);
extern void update_safety(void);

static inline bool movement_okay(void);
static inline bool main_laser_okay(void);
static inline bool visible_laser_okay(void);

static inline bool stop_button_is_down(void);
static inline bool lid_is_open(void);


// Implementation

#include <stdint.h>

enum {
    stop_switch_down = 1 << 0,
    lid_switch_open = 1 << 1,
    
};

extern struct safety_private {
    volatile uint8_t state;
    volatile bool move_ok;
    volatile bool main_ok;
    volatile bool vis_ok;
} safety_private;

static inline bool lid_is_open(void)
{
    return safety_private.state & lid_switch_open;
}

static inline bool stop_button_is_down(void)
{
    return safety_private.state & stop_switch_down;
}

static inline bool movement_okay(void)
{
    return safety_private.move_ok;
}

static inline bool main_laser_okay(void)
{
    return safety_private.main_ok;
}

static inline bool visible_laser_okay(void)
{
    return safety_private.vis_ok;
}

#endif /* !SAFETY_included */
