#ifndef FW_ASSERT_included
#define FW_ASSERT_included

#if 0
// fw_assert() is like libc's assert() but sends an S-code packet.

#ifdef FW_NDEBUG

#define fw_assert(expr) ((void)0)

#else

#define fw_assert(expr) ((expr) ? (void)0 : fw_assertion_failed(__LINE__))

extern void fw_assertion_failed(unsigned int line_no) __attribute__((noreturn));

#endif

#else

#include <assert.h>
#define fw_assert assert

#endif

#endif /* !FW_ASSERT_included */
