#ifndef ENGINE_included
#define ENGINE_included

typedef enum engine_state {
    ES_STOPPED,
    ES_RUNNING,
    ES_STOPPING
} engine_state;

extern void init_engine(void);  // Why not?

extern engine_state get_engine_state(void);

extern void maybe_start_engine(void);
extern void start_engine(void);

extern void stop_engine_immediately(void);
extern void await_engine_stopped(void);

#endif /* !ENGINE_included */
