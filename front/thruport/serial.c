#include "serial.h"

#include <pthread.h>
#include <stdlib.h>
#include <syslog.h>
#include <termios.h>
#include <sys/fcntl.h>

#include "debug.h"
#include "io.h"
#include "lock.h"
#include "paths.h"

static struct termios  orig_termios, raw_termios;
static size_t          tty_bufsize;
static char           *tty_rawbuf;
static int             ttyfd       = -1;

static pthread_mutex_t serial_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  serial_cond = PTHREAD_COND_INITIALIZER;
static size_t          tx_sent, tx_received, tx_space;

static void make_raw(struct termios *tiosp)
{
    tiosp->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                        INLCR | IGNCR | ICRNL | IXON);
    tiosp->c_oflag &= ~ OPOST;
    tiosp->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tiosp->c_cflag &= ~(CSIZE | PARENB);
    tiosp->c_cflag |=   CS8;
}

static void restore_serial(void)
{
    if (ttyfd != -1) {
        (void)tcsetattr(ttyfd, TCSANOW, &orig_termios);
        (void)close(ttyfd);
        ttyfd = -1;
    }
}

int init_serial(void)
{
    // Get the lock.
    int r = lock_serial_port();
    if (r < 0)
        return r;

    // Open the serial device.
    const char *dev = get_device();
    //int ttyfd = open(dev, O_RDWR | O_NONBLOCK | O_NOCTTY, 0666);
    ttyfd = open(dev, O_RDWR | O_NOCTTY, 0666);
    if (ttyfd < 0) {
        syslog(LOG_ERR, "can't open %s: %m", dev);
        (void)unlock_serial_port();
        return -1;
    }

    // Get and save the mode bits.
    if (tcgetattr(ttyfd, &orig_termios)) {
        syslog(LOG_ERR, "tcgetattr failed: %m");
        (void)close(ttyfd);
        (void)unlock_serial_port();
        return -1;
    }

    // Set device to raw mode.
    raw_termios = orig_termios;
    make_raw(&raw_termios);
    if (tcsetattr(ttyfd, TCSAFLUSH, &raw_termios)) {
        syslog(LOG_ERR, "tcsetattr failed: %m");
        (void)close(ttyfd);
        (void)unlock_serial_port();
        return -1;
    }

    // Init I/O buffers.
    tty_bufsize = fd_blksize(ttyfd);
    tty_rawbuf  = malloc(tty_bufsize);
    if (!tty_rawbuf) {
        syslog(LOG_CRIT, "malloc failed: %m");
        exit(EXIT_FAILURE);
    }

    // Register handlers.
    atexit(restore_serial);
    return 0;
}

int serial_transmit(const char *buf, size_t size)
{
    pthread_mutex_lock(&serial_lock);
    while (size) {
        while (!tx_space) {
            DBG("wait");
            pthread_cond_wait(&serial_cond, &serial_lock);
        }
        size_t ntw = tx_space;
        if (ntw > size)
            ntw = size;
        tx_sent += ntw;
        DBG("tx_space %zu -> %zu", tx_space, tx_space - ntw);
        tx_space -= ntw;
        while (ntw) {
            pthread_mutex_unlock(&serial_lock);
            DBG("write %s", str_repr(buf, ntw, false));
            ssize_t nw = write(ttyfd, buf, ntw);
            DBG("nw = %zd, ntw = %zu, tx_space = %zu", nw, ntw, tx_space);
            pthread_mutex_lock(&serial_lock);
            if (nw < 0) {
                tx_sent -= ntw;
                DBG("tx_space %zu -> %zu", tx_space, tx_space + ntw);
                tx_space += ntw;
                pthread_mutex_unlock(&serial_lock);
                DBG("wwr error %m");
                return 1;
            }
            ntw -= nw;
            size -= nw;
            buf += nw;
        }
    }
    pthread_mutex_unlock(&serial_lock);
    return 0;
}

static inline bool eat_flow_char(char c)
{
    if ((c & 0xF0) == 0xF0) {
        pthread_mutex_lock(&serial_lock);
        size_t olo = tx_received & 0xFF;
        size_t ohi = tx_received & ~0xFF;
        size_t nlo = (size_t)(c & 0x0F) << 4;
        size_t nhi = ohi;
        if (nlo < olo)
            nhi += 0x100;
        tx_received = nhi | nlo;
        DBG("tx_space %zu -> %zu", tx_space, tx_received + 0xFF - tx_sent);
        tx_space = tx_received + 0xFF - tx_sent;
        if (tx_space) {
            DBG("signal");
            pthread_cond_signal(&serial_cond);
        }
        pthread_mutex_unlock(&serial_lock);
        return true;
    }
    else
        return false;
}

static size_t cook_chars(char *canon, const char *raw, size_t raw_count)
{
    size_t ncanon = 0;
    for (size_t i = 0; i < raw_count; i++) {
        char c = raw[i];
        if (!eat_flow_char(c))
            canon[ncanon++] = c;
    }
    return ncanon;
}

ssize_t serial_receive(char *buf, size_t max)
{
    while (true) {
        ssize_t nread;
        nread = read(ttyfd, tty_rawbuf, max);
        if (nread < 0) {
            syslog(LOG_ERR, "tty read failed: %m");
            return nread;
        }
        if (nread > 0) {
            size_t ncanon = cook_chars(buf, tty_rawbuf, nread);
            static size_t raw_tot = 0, can_tot = 0;
            // DBG("read %zu + %zd raw, %zu + %zu canon",
            //     raw_tot, nread, can_tot, ncanon);
            raw_tot += nread; can_tot += ncanon;
            if (ncanon > 0)
                return ncanon;
        }
    }
}

