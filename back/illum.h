#ifndef ILLUM_included
#define ILLUM_included

#include <stdint.h>

typedef enum animation_index {
    A_START,
    A_COMPLETE,
    A_WARNING,
    A_ALERT,
    A_NONE,
    ANIMATION_COUNT
} animation_index, a_index;

extern void init_illumination(void);
extern void start_animation(uint8_t anim);

#endif /* !ILLUM_included */
