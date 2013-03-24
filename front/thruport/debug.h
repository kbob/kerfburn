#ifndef DEBUG_included
#define DEBUG_included

#define DBG(...) \
    (debug_printf(__FILE__, __LINE__, __func__, __VA_ARGS__))

#define HELLO (DBG(NULL))

__attribute__((format(printf, 4, 5)))
extern void debug_printf(const char *file, int line, const char *func,
                         const char *fmt, ...);

#endif /* !DEBUG_included */
