#ifndef IO_included
#define IO_included

// Miscellaneous I/O functions.

#include <stdbool.h>
#include <unistd.h>

extern size_t  fd_blksize       (int fd);

extern ssize_t read_line        (int fd, char *buf, size_t count);

extern const char *char_repr    (int c, bool highlighted);
extern const char *str_repr     (const char *s, size_t max, bool highlighted);

#endif /* !IO_included */
