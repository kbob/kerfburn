#ifndef SERIAL_INCLUDED
#define SERIAL_INCLUDED

#include <stdbool.h>

#include <avr/io.h>

typedef enum serial_error_bit {
    SE_OK           = 0,
    SE_NO_DATA      = _BV(UDRE0),
    SE_FRAME_ERROR  = _BV(FE0),
    SE_DATA_OVERRUN = _BV(DOR0),
    SE_PARITY_ERROR = _BV(UPE0),
} serial_error_bit;

extern void    init_serial(void);

extern void     serial_rx_start        (void);
extern uint8_t  serial_rx_errors       (void);
extern uint8_t  serial_rx_peek_errors  (void);
extern bool     serial_rx_has_chars    (void);
extern bool     serial_rx_has_lines    (void);
extern uint16_t serial_rx_char_count   (void);
extern uint8_t  serial_rx_peek_char    (uint16_t pos);
extern void     serial_rx_consume      (uint16_t count);

extern uint8_t  serial_tx_errors       (void);
extern uint8_t  serial_tx_peek_errors  (void);
extern bool     serial_tx_is_idle      (void);
extern uint8_t  serial_tx_is_available (void);
extern bool     serial_tx_put_char     (uint8_t c);

#endif /* !SERIAL_INCLUDED */
