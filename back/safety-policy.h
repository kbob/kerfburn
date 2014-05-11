#ifndef SAFETY_POLICY_included
#define SAFETY_POLICY_included

#include <stdbool.h>

extern void emergency_stop(void);
extern bool is_emergency_stopped(void);

#endif /* !SAFETY_POLICY_included */
