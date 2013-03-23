#ifndef SERIAL_included
#define SERIAL_included

#include <stddef.h>

extern int init_serial(void);

extern char *serial_get_write_buf(size_t *size_out);
extern void serial_send_data(size_t count);

#endif /* !SERIAL_included */
