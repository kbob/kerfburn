#ifndef RECEIVER_SERVICE_included
#define RECEIVER_SERVICE_included

#include <stddef.h>

extern void instantiate_receiver_service(int sock);

extern void broadcast_to_receivers(const char *data, size_t count);

#endif /* !RECEIVER_SERVICE_included */
 
