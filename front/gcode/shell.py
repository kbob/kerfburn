#!/usr/bin/python

import sys
import time
import traceback

import gcode
import gcode.parser

def main(argv):
    print >>sys.stderr, 'argv=%r' % argv
    interp = gcode.Interpreter()
    for file in argv[1:]:
        try:
            f = open(file)
        except IOError:
            traceback.print_exc(0)
            continue
        try:
            while True:
                a = interp.interpret_file(f)
                if a is None:
                    break
                print >>sys.stderr, a
                if a in ('End', 'Emergency Stop'):
                    break
                if 'Pause' in a:
                    time.sleep(1)
        except gcode.GCodeSyntaxError:
            traceback.print_exc(0)

if __name__ == '__main__':
    main(sys.argv)
