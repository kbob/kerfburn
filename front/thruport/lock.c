#include "lock.h"

#include <sys/fcntl.h>

#include "paths.h"

int lock_serial_port(void)
{
    // const char *lck_path = get_lock_path();
    // int fd = open(lck_path, O_CREAT | O_EXCL | O_TRUNC, 0600);
    return 0;
}

int unlock_serial_port(void)
{
    // const char *lck_path = get_lock_file();
    return 0;
}

