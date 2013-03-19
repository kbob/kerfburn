#!/usr/bin/python

import string
import sys
import time

def test_pattern():
    w = [c * 4 for c in string.lowercase]
    lines = [' '.join(w[i:i+4]) + '\n' for i in range(0, len(w), 4)]
    return ''.join(lines)


while True:
    # print >>sys.stderr, "blort"
    print test_pattern()
    sys.stdout.flush()
    # time.sleep(0.3)
