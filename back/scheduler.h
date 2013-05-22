#ifndef SCHEDULER_included
#define SCHEDULER_included

extern void init_scheduler   (void);

extern void enqueue_dwell    (void);
extern void enqueue_move     (void);
// extern void enqueue_cut      (void);
extern void enqueue_engrave  (void);
extern void enqueue_home     (void);

extern void stop_immediately (void);
extern void await_completion (void);

#endif /* !SCHEDULER_included */
