#!/usr/bin/python

"""Command-line shell for G-Code.

   If standard input and standard output are a terminal, then it
   interactively prompts for and executes commands.  Otherwise, it
   reads from files on command line or from standard output.
"""

import argparse
import atexit
import os
import sys
import time
import traceback

import gcode
import gcode.parser


HISTORY_FILE_TEMPLATE = '~/.gcode-history'

program_start_time = time.time()


def open_files(files):
    if files:
        for file in files:
            try:
                yield open(file)
            except IOError:
                traceback.print_exc(0)
                break
    else:
        yield sys.stdin

def cat(files):
    interp = gcode.Interpreter()
    for f in open_files(files):
        try:
            while True:
                a = interp.interpret_file(f)
                if a is None:
                    break
                print >>sys.stderr, a
                if a in ('End', 'Emergency Stop'):
                    break
                if 'Pause' in a:
                    print >>sys.stderr, 'Pause'
                    time.sleep(1)
        except gcode.GCodeSyntaxError:
            traceback.print_exc(0)
            break


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
            argv = [sys.executable, '-m', 'gcode.shell'] + sys.argv[1:]
            
            sys.exitfunc()
            sys.exitfunc = None # Don't call it again if execv fails.
            os.execv(argv[0], argv)


def interact():

    import readline

    history_file = os.path.expanduser(HISTORY_FILE_TEMPLATE)
    try:
        readline.read_history_file(history_file)
    except IOError:
        pass
    atexit.register(readline.write_history_file, history_file)

    interp = gcode.Interpreter()
    while True:
        try:
            line = raw_input('> ')
        except (EOFError, KeyboardInterrupt):
            print
            return
        restart_as_needed()
        try:
            sline = gcode.SourceLine(line, source='<tty>')
            action = interp.interpret_line(sline)
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


def is_interactive():
    return all(os.isatty(f.fileno()) for f in (sys.stdin, sys.stdout))

def main(argv):
    desc = 'G-Code command-line shell'
    p = argparse.ArgumentParser(description=desc)
    p.add_argument('file', nargs='*', help='G-Code source file')
    args = p.parse_args()
    if args.file or not is_interactive():
        cat(args.file)
    else:
        interact()

if __name__ == '__main__':
    main(sys.argv)
