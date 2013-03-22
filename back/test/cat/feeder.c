#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>

#define BACK_RX_BUF_SIZE 256
#define FLOW_SHIFT 4

struct termios orig_tios, raw_tios;
char     buf0[BUFSIZ];
size_t   count0;
uint8_t  tx_mark;
uint16_t tx_received;
uint16_t tx_limit;
uint16_t tx_count;

char     buf1[BUFSIZ];
size_t   count1;

void pstat(int fd, const char *label)
{
    struct stat s;
    if (fstat(fd, &s) < 0)
        perror(label);
    else
        printf("%s: blksize = %ld\n", label, (long)s.st_blksize);
}

int putc_readable(char c, FILE *f)
{
    char c1 = c;
    if ((' ' <= c && c < '\177') || c == '\n')
#if 1
        return putc(c1, f);
#else
        return 0;
#endif
    const char *mp = "";
    if ((uint8_t)c1 >= 0xF0) {
        return fprintf(f, "\33[1mM%x\33[m", c1 & 0x0F);
    } else if (c1 & 0200) {
        c1 &= ~0200;
        mp = "M-";
        if (' ' <= c1 && c1 < '\177')
            return fprintf(f, "\033[1mM-%c\033[m", c1) == EOF ? EOF : c;
    }
    return fprintf(f, "\033[1m%s^%c\033[m", mp, c1 ^ 0100) == EOF ? EOF : c;
}

void calc_limit(uint8_t mark)
{
    uint8_t lo = tx_received & 0x00FF;
    uint16_t hi = tx_received & 0xFF00;
    if (mark < lo)
        hi += 0x0100;
    // printf("calc tx_received %#04x -> %#04x, tx_limit %#04x -> %#04x\n",
    //        tx_received, (hi | mark), tx_limit, (hi | mark) + 0xFF);
    fflush(stdout);
    tx_received = hi | mark;
    tx_limit = tx_received + 0xFF;
}

uint8_t tx_space(void)
{
    return tx_limit - tx_count;
}

int main()
{
    int infd = fileno(stdin);
    int outfd = fileno(stdout);
    int ttyfd = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK | O_NOCTTY, 0666);
    if (ttyfd < 0)
        perror("/dev/ttyUSB0"), exit(EXIT_FAILURE);

    pstat(infd, "standard input");
    pstat(outfd, "standard output");
    pstat(ttyfd, "serial");

    if (tcgetattr(ttyfd, &orig_tios) != 0)
        perror("tcgetattr"), exit(EXIT_FAILURE);
    raw_tios = orig_tios;
    raw_tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                          INLCR | IGNCR | ICRNL | IXON);
    raw_tios.c_oflag &= ~ OPOST;
    raw_tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw_tios.c_cflag &= ~(CSIZE | PARENB);
    raw_tios.c_cflag |=   CS8;
    if (tcsetattr(ttyfd, TCSAFLUSH, &raw_tios) != 0)
        perror("tcsetattr"), exit(EXIT_FAILURE);

    while (true) {
        int nfds = ttyfd + 1;;
        fd_set readfds, writefds;
        fd_set *writefdp = NULL;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(ttyfd, &readfds);
        if (count0 && tx_space()) {
            writefdp = &writefds;
            FD_SET(ttyfd, writefdp);
        } else {
            FD_SET(infd, &readfds);
            if (infd >= nfds)
                nfds = infd + 1;
        }
        int r_nfds = select(nfds, &readfds, writefdp, NULL, NULL);
        if (r_nfds < 0)
            perror("select"), exit(EXIT_FAILURE);

        if (FD_ISSET(ttyfd, &readfds)) {

            // Read from serial (Highest priority)
            ssize_t nr = read(ttyfd, buf1, sizeof buf1);
            if (nr == -1)
                perror("read tty"), exit(EXIT_FAILURE);
            else {
                size_t i;
                for (i = 0; i < nr; i++) {
                    char c = buf1[i];
                    uint8_t uc = (uint8_t)c;
                    if ((uc & 0xF0) == 0xF0) {
                        tx_mark = (uc & ~-(1 << FLOW_SHIFT)) << FLOW_SHIFT;
                        calc_limit(tx_mark);
                    } else {
                        putc_readable(c, stdout);
                        fflush(stdout);
                    }
                }
            }
        } else if (!count0 && FD_ISSET(infd, &readfds)) {

            // Read from stdin.
            ssize_t nr = read(infd, buf0, sizeof buf0);
            if (nr < 0)
                perror("read stdin"), exit(EXIT_FAILURE);
            count0 = nr;
            // printf("read %zd; count0=%zd\n", nr, count0);
        }
        else if (count0 && tx_space() && FD_ISSET(ttyfd, writefdp)) {

            // Write to serial.
            size_t ntw = tx_space();
            if (ntw > count0)
                ntw = count0;
            ssize_t nw = write(ttyfd, buf0, ntw);
            if (nw < 0)
                perror("write"), exit(EXIT_FAILURE);
            else {
                if (nw && nw < count0)
                    memmove(buf0, buf0 + nw, count0 - nw);
                count0 -= nw;
                tx_count += nw;
                // printf("writ %zd; count0=%zd\n", nw, count0);
            }
        }
    }

    if (tcsetattr(ttyfd, TCSAFLUSH, &orig_tios) != 0)
        perror("tcsetattr"), exit(EXIT_FAILURE);
    return EXIT_SUCCESS;
}
