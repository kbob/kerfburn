#ifndef ENGINE_included
#define ENGINE_included

extern void init_engine(void);  // Why not?

extern void start_engine(void);

extern void stop_engine_immediately(void);
extern void await_engine_stopped(void);

#endif /* !ENGINE_included */
