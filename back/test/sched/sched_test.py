"""sched_test"""

import argparse
import re
from subprocess import call, Popen, PIPE
import tempfile
import sys

def run_command(input):
    fw = Popen(['./fw'], stdin=PIPE, stdout=PIPE, stderr=PIPE)
    out, err = fw.communicate(input)
    if err:
        print >>sys.stderr, err,
        exit(1)
    out = re.sub(r'built \d.*', 'built [...]', out, 1)
    return out

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
    actual = run_command(input)
    print actual,

def show_expected(expected):
    print expected,

def run_test(input, expected, diff):
    if diff is None:
        diff = 'diff'
    actual = run_command(input)
    if actual != expected:
        show_diff(expected, actual, diff)
        exit(1)

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

