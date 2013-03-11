#ifndef FW_ASSERT_included
#define FW_ASSERT_included

// fw_assert() is like libc's assert() but sends an S-code packet.

#ifdef FW_NDEBUG

#define fw_assert(expr) ((void)0)

#else

#define fw_assert(expr) ((expr) ? (void)0 : fw_assertion_failed(__LINE__))

extern void fw_assertion_failed(int line_no) __attribute__((noreturn));

#endif

#endif /* !FW_ASSERT_included */
