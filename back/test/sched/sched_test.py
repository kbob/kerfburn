"""sched_test"""

import argparse
import re
import signal
from subprocess import call, Popen, PIPE
import tempfile
import sys


class Unset: pass               # Just a unique value


phase = 'Unknown'


class AxisState(object):

    def __init__(self, name):
        self.name = name
        self.time = 0

    def do(self, atom):
        try:
            n = int(atom)
        except ValueError:
            n = None
        if n is None:
            self.do_special(atom)
        else:
            self.do_numeric(n)

    def done(self):
        # Any tests to run after all done?
        pass

    def fail(self, msg):
        exit('%s %s: %s' % (phase, self.name, msg))


class MotorAxisState(AxisState):

    def __init__(self, name):
        super(MotorAxisState, self).__init__(name)
        self.distance = 0
        self.enabled = Unset
        self.direction = Unset

    def do_numeric(self, num):
        if self.enabled is Unset:
            self.fail('Tried to step before enabling/disabling')
        if self.enabled:
            if self.direction is Unset:
                self.fail('Tried to step before setting direction')
            self.distance += self.direction
        self.time += num

    def do_special(self, atom):
        if atom == 'A_DIR_POSITIVE':
            self.direction = +1
        elif atom == 'A_DIR_NEGATIVE':
            self.direction = -1
        elif atom.startswith('A_WHILE_') or atom.startswith('A_UNTIL_'):
            assert False, "can't handle loops"
        elif atom == 'A_ENABLE_STEP':
            self.enabled = True
        elif atom == 'A_DISABLE_STEP':
            self.enabled = False
        else:
            assert False, 'unknown atom %r' % atom


class LaserAxisState(AxisState):

    class State:                # state machine
        normal        = 0
        main_duration = 1
        vis_duration  = 2
        power_level   = 3

    class Mode:                 # laser modes
        off           = 0
        pulsed        = 1
        continuous    = 2

    def __init__(self, name):
        super(LaserAxisState, self).__init__(name)
        self.state         = self.State.normal
        self.main_mode     = Unset
        self.main_duration = Unset
        self.main_power    = Unset
        self.main_count    = 0
        self.vis_mode      = Unset
        self.vis_duration  = Unset
        self.vis_count     = 0

    def do_numeric(self, num):
        if self.state == self.State.normal:
            self.do_pulse(num)
        elif self.state == self.State.main_duration:
            self.main_duration = num
        elif self.state == self.State.vis_duration:
            self.vis_duration = num
        elif self.state == self.State.power_level:
            self.main_power = num
        else:
            assert False, 'unknown state %r' % self.state
        self.state = self.State.normal

    def do_special(self, atom):
        if atom == 'A_SET_MAIN_LASER_OFF':
            self.main_mode = self.Mode.off
        elif atom == 'A_SET_MAIN_LASER_PULSED':
            self.main_mode = self.Mode.pulsed
        elif atom == 'A_SET_MAIN_LASER_CONTINUOUS':
            if self.main_power is Unset:
                self.fail('Tried to fire main laser before setting power level')
            self.main_mode = self.Mode.continuous
        elif atom == 'A_SET_VISIBLE_LASER_OFF':
            self.vis_mode = self.Mode.off
        elif atom == 'A_SET_VISIBLE_LASER_PULSED':
            self.vis_mode = self.Mode.pulsed
        elif atom == 'A_SET_VISIBLE_LASER_CONTINUOUS':
            self.vis_mode = self.Mode.continuous
        elif atom == 'A_SET_MAIN_PULSE_DURATION':
            self.state = self.State.main_duration
        elif atom == 'A_SET_VISIBLE_PULSE_DURATION':
            self.state = self.State.vis_duration
        elif atom == 'A_SET_MAIN_POWER_LEVEL':
            self.state = self.State.power_level
        else:
            assert False, 'unknown atom %r' % atom

    def do_pulse(self, num):
        self.time += num

        if self.main_mode is Unset:
            self.fail('Tried to pulse before setting main mode')
        if self.vis_mode is Unset:
            self.fail('Tried to pulse before setting visible mode')
        if self.main_mode != self.Mode.off and self.vis_mode != self.Mode.off:
            self.fail('Tried to pulse with both lasers on')

        if self.main_mode != self.Mode.off and self.main_power is Unset:
            self.fail('Tried to fire main laser before setting power level')
        if self.main_mode == self.Mode.pulsed and self.main_duration is Unset:
            self.fail('Tried to pulse main laser before setting duration')

        if self.vis_mode == self.Mode.pulsed and self.vis_duration is Unset:
            self.fail('Tried to pulse visible laser before setting duration')


def check_answer(input, output):
    axis_states = [MotorAxisState(a) for a in 'XYZ'] + [LaserAxisState('P')]
    ax_dict = dict((a.name, a) for a in axis_states)

    for line in output.split('\n'):
        m = re.match(r'enqueue_atom_([XYZP]): a = (.*)\Z', line)
        if m:
            ax_dict[m.group(1)].do(m.group(2))

    for axis in axis_states:
        axis.done()
    if not all(a.time == axis_states[0].time for a in axis_states):
        print >>sys.stderr, '%s: Times mismatch.' % phase
        for axis in axis_states:
            print >>sys.stderr, '    %s: %d' % (axis.name, axis.time)
        exit(1)


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


def run_test(input, expected, diff, extra_axes):
    if diff is None:
        diff = 'diff'
    global phase
    phase = 'Verifying Expected Result'
    check_answer(input, expected)
    returncode, actual, err = run_command(input)
    if not returncode and not err and actual != expected:
        phase = 'Actual Result'
        check_answer(input, actual)
        show_diff(expected, actual, diff)
        exit(1)
    show_exit(returncode, actual, err)


# def run_test_extra(input, expected, diff, extra):
#     if extra:
#         axes = count_active_axes(input)
#         if axe
        

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
    parser.add_argument('-d', '--diff',
                        help='use diff program')
    parser.add_argument('-x', '--extra-axes',
                        action='store_true',
                        help='use all axes/pairs')
    args = parser.parse_args()
    
    if args.input:
        show_input(input)
    elif args.actual:
        show_actual(input)
    elif args.expected:
        show_expected(expected)
    else:
        run_test(input, expected, args.diff, args.extra_axes)

