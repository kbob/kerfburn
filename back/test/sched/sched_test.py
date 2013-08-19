"""sched_test"""

import argparse
import re
import signal
from subprocess import call, Popen, PIPE
import tempfile
import sys

def strsignal(sig):
    signames = dict((getattr(signal, name), name)
                    for name in dir(signal)
                    if re.match(r'SIG[^_]', name))
    return signames.get(sig, 'Unknown signal %d' % sig)

def show_exit(returncode, out, err):
    if err:
        exit(err)
    if returncode:
        if returncode < 0:
            exit('./fw killed with %s' % strsignal(-returncode))
        else:
            exit('exit status: %d' % returncode)
    exit(0)

def run_command(input):
    fw = Popen(['./fw'], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    out, err = fw.communicate(input)
    out = re.sub(r'built \d.*', 'built [...]', out, 1)
    return fw.returncode, out, err

def show_diff(expected, actual, diff):
    ef = tempfile.NamedTemporaryFile(prefix='expected.')
    ef.write(expected)
    ef.flush()
    af = tempfile.NamedTemporaryFile(prefix='actual.')
    af.write(actual)
    af.flush()
    call([diff, ef.name, af.name])


def show_input(input):
    print input,

def show_actual(input):
    returncode, actual, err = run_command(input)
    print actual,
    show_exit(returncode, actual, err)

def show_expected(expected):
    print expected,

def run_test(input, expected, diff):
    if diff is None:
        diff = 'diff'
    returncode, actual, err = run_command(input)
    if not returncode and not err and actual != expected:
        show_diff(expected, actual, diff)
        exit(1)
    show_exit(returncode, actual, err)

def test(input, expected):

    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-i', '--input',
                       action='store_true',
                       help='show input')
    group.add_argument('-a', '--actual',
                       action='store_true',
                       help='show actual result')
    group.add_argument('-e', '--expected',
                       action='store_true',
                       help='show expected result')
    group.add_argument('-d', '--diff',
                       help='use diff program')
    args = parser.parse_args()
    
    if args.input:
        show_input(input)
    elif args.actual:
        show_actual(input)
    elif args.expected:
        show_expected(expected)
    else:
        run_test(input, expected, args.diff)

