#ifndef LOCK_included
#define LOCK_included

// These return zero on success, set errno on failure.
extern int lock_serial_port   (void);
extern int unlock_serial_port (void);

#endif /* !LOCK_included */
