#ifndef E_STOP_included
#define E_STOP_included

#include <stdbool.h>

extern void init_emergency_stop(void);

extern bool is_emergency_stopped(void);

extern void emergency_stop_NONATOMIC(void); // XXX should be inline.
extern void emergency_stop(void);

#endif /* !E_STOP_included */
