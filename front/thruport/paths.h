#ifndef PATHS_included
#define PATHS_included

extern void        set_port           (const char *port);
extern const char *get_device         (void);
extern const char *get_socket_dir     (void);
extern const char *get_socket_path    (void);
extern const char *get_lock_dir       (void);
extern const char *get_lock_path      (void);

#endif /* !PATHS_included */
