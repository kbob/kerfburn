#!/usr/bin/python

import atexit
import os
import readline
import sys
import time
import traceback

import gcode


program_start_time = time.time()

HISTORY_FILE = os.path.expanduser('~/.gcode-history')
try:
    readline.read_history_file(HISTORY_FILE)
except IOError:
    pass
atexit.register(readline.write_history_file, HISTORY_FILE)


def restart_as_needed():
    for (name, mod) in sys.modules.iteritems():
        if name != '__main__' and not name.startswith('gcode'):
            continue
        if mod is None:
            continue
        file = mod.__file__
        if file.endswith('.pyc'):
            file = file[:-1]
        if os.stat(file).st_mtime > program_start_time:
            print >>sys.stderr, '%s changed, restarting' % file
            argv = [sys.executable] + sys.argv
            sys.exitfunc()
            os.execv(argv[0], argv)


def main_loop():
    interp = gcode.Interpreter()
    while True:
        try:
            line = raw_input('> ')
        except (EOFError, KeyboardInterrupt):
            print
            break
        restart_as_needed()
        try:
            action = interp.interpret_line(line)
            if action:
                print 'ACTION', action
        except gcode.GCodeSyntaxError:
            z = traceback.format_exc(0)
            z = z.split('\n', 3)[3]   # discard first three lines
            z = z.replace('  ^', '^') # move caret left two columns
            z = z.rstrip()            # discard trailing newline
            print >>sys.stderr, z
        except gcode.GCodeException:
            z = traceback.format_exc(0)
            z = z.split('\n', 1)[1]   # discard first line
            z = z.rstrip()            # discard trailing newline
            print >>sys.stderr, z

if __name__ == '__main__':
    main_loop()
