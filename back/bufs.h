#ifndef BUFS_included
#define BUFS_included

#include <stdint.h>

#define BCNT 6

// We use several buffers of 256 bytes, all aligned on a 256 byte
// boundary.  They are declared here as aliases to the same memory
// chunk so that they are allocated contigously.

// All buffers contiguous
extern char all_bufs[BCNT][256] __attribute__((aligned(256)));

#define tx_buf ((uint8_t  *)all_bufs[0])
#define rx_buf ((uint8_t  *)all_bufs[1])
#define Xq_buf ((uint16_t *)all_bufs[2])
#define Yq_buf ((uint16_t *)all_bufs[3])
#define Zq_buf ((uint16_t *)all_bufs[4])
#define Pq_buf ((uint16_t *)all_bufs[5])

#endif /* !BUFS_included */
