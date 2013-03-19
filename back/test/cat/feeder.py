#!/usr/bin/python

from contextlib import contextmanager
import os
import string
import sys
import termios


@contextmanager
def raw_mode(fd, when=termios.TCSAFLUSH):
    """Put terminal into a raw mode."""
    # Indexes for termios list.
    IFLAG = 0
    OFLAG = 1
    CFLAG = 2
    LFLAG = 3
    ISPEED = 4
    OSPEED = 5
    CC = 6
    saved_mode = termios.tcgetattr(fd)
    raw_mode = saved_mode[:]

    cc = raw_mode[CC]
    print 'CC', cc
    print 'VSTOP %s' % readable(cc[termios.VSTOP])
    print 'VSTART %s' % readable(cc[termios.VSTART])
    print 'CRTSCTS %#x' % (raw_mode[CFLAG] & termios.CRTSCTS)

    raw_mode[IFLAG] &= ~(termios.BRKINT | termios.ICRNL |
                         termios.INPCK | termios.ISTRIP)
  # raw_mode[IFLAG] |=  (termios.IXON | termios.IXOFF) # XXX kbob
  # raw_mode[IFLAG] |=   termios.IXON # XXX kbob
    raw_mode[IFLAG] &= ~(termios.IXON) # XXX kbob
    raw_mode[OFLAG] &= ~(termios.OPOST)
    raw_mode[CFLAG] &= ~(termios.CSIZE | termios.PARENB)
    raw_mode[CFLAG] |=  termios.CS8
#   raw_mode[CFLAG] &= ~(termios.CRTSCTS) # XXX kbob
    raw_mode[LFLAG] &= ~(termios.ECHO | termios.ICANON |
                         termios.IEXTEN | termios.ISIG)
    raw_mode[CC][termios.VMIN]  = 1
    raw_mode[CC][termios.VTIME] = 1
    termios.tcsetattr(fd, when, raw_mode)
    try:
        yield 
    finally:
        termios.tcsetattr(fd, when, saved_mode)

@contextmanager
def os_open(file, name, mode=0666):
    fd = os.open(file, name, mode)
    try:
        yield fd
    finally:
        os.close(fd)

def readable(c):
    i = ord(c)
    if 32 <= i < 0177 or c in '\n':
        return c
    return '\033[1m^%c\033[m' % chr(i ^ 0100)

def accum_input(ttyfd):
    data = ''
    while True:
        try:
            datum = os.read(ttyfd, 1)
        except OSError, x:
            print >>sys.stdout, x
            continue
        os.write(sys.stdout.fileno(), readable(datum))
        data += datum
        yield data

def test_pattern():
    w = [c * 4 for c in string.lowercase]
    lines = [' '.join(w[i:i+4]) + '\r\n' for i in range(0, len(w), 4)]
    return ''.join(lines)

def test_cat():
    print '[feeder]'
    sent = False
    with os_open('/dev/ttyUSB0', os.O_RDWR) as ttyfd, raw_mode(ttyfd):
        for data in accum_input(ttyfd):
            if not sent and 'Ready\r\n' in data:
                print 'Found ready prompt'
                os.write(ttyfd, test_pattern() * 2)
                sent = True

if __name__ == '__main__':
    try:
        test_cat()
    except KeyboardInterrupt:
        print
        exit(1)

