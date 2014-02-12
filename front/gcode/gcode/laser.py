"""G-Code Machine Protocol for laser cutter"""

from math import sqrt
import sys

from gcode.arc import XYArcMixin
from gcode.core import ApproximateNumber, CheapEnum, Executor, GCodeException
from gcode.core import code, group_prepare, group_finish
from gcode.core import modal_group, nonmodal_group
from gcode.motion import DistanceMode, DistanceUnits
from gcode.parser import parse_comment


F_CPU = 16000000                # CPU frequency - should come from config.
TRAVERSE_RATE = 100             # mm/sec
DEFAULT_FEED_RATE = 25          # mm/sec
X_USTEPS_PER_INCH = 2000
Y_USTEPS_PER_INCH = 2000
Z_USTEPS_PER_INCH = 20825


class Animation(CheapEnum):
    startup = 's'
    complete = 'c'
    warning = 'w'
    alert = 'a'
    none = 'n'
    _map = {
        0: none,
        1: startup,
        2: complete,
        3: warning,
        4: alert,
        }

class LaserSelect(CheapEnum):
    none = 'n'
    main = 'm'
    visible = 'v'
    _map = {
        0: none,
        1: main,
        2: visible,
        }

class PulseMode(CheapEnum):
    continuous = 'c'
    timed = 't'
    distance = 'd'
    off = 'o'


class AxisPosition(object):

    def __init__(self, usteps_per_inch):
        self.ustep_conv_map = {
            DistanceUnits.inch: usteps_per_inch,
            DistanceUnits.mm: usteps_per_inch / 25.4,
            }
        self.reset()

    def reset(self):
        self.pos_units = 0
        self.pos_usteps = 0

    def update_pos_usteps(self, units):
        self.pos_usteps = usteps = self.units_to_usteps(self.pos_units, units)
        return usteps

    def units_to_usteps(self, pos, units, integer=True):
        C = self.ustep_conv_map[units]
        usteps = pos * C
        if (integer):
            usteps = int(round(usteps))
        return usteps


class LaserExecutor(Executor, XYArcMixin):

    def __init__(self):
        self.distance_units = DistanceUnits.mm
        self.distance_mode = DistanceMode.absolute
        self.abs_position_known = False
        self.x_pos = AxisPosition(X_USTEPS_PER_INCH)
        self.y_pos = AxisPosition(Y_USTEPS_PER_INCH)
        self.z_pos = AxisPosition(Z_USTEPS_PER_INCH)
        self.traverse_ivl_native = self.native_ivl(TRAVERSE_RATE)
        self.feed_ivl_native = self.native_ivl(DEFAULT_FEED_RATE)
        self.pulse_mode = PulseMode.off

    @property
    def initial_settings(self):
        d = dict(super(LaserExecutor, self).initial_settings)
        d.update({
                'F': DEFAULT_FEED_RATE * 60, # mm/sec -> mm/min)
                'P': None,
                'S': None,
                'T': None,
                'X': None,
                'Y': None,
                'Z': None,
                })
        return d

    @property
    def order_of_execution(self):
        return (
            self.exec_begin_line,
            'emergency stop',
            self.exec_comment,
            'low voltage',
            'illumination',
            'tool change',
            'water',
            'air',
            'high voltage',
            'laser power',
            'laser pulse mode',
            'laser pulse width',
            'motors',
            'dwell',
            'plane selection',
            'units',
            'distance mode',
            'home',
            'motion',
            'stop',
            )

    def exec_begin_line(self, settings, new_settings, pline):
        self.line_already_used_axes = False

    def exec_comment(self, settings, new_settings, pline):
        if pline.comment:
            hdr, rest = parse_comment(pline.source.pos, pline.comment)
            if hdr == 'MSG':
                print >>sys.stderr, 'MSG', rest

    # #  #    #    #     #      #       #      #     #    #   #  # #

    with modal_group('motion'):

        @code(require_any='XYZ')
        def G0(self, X=None, Y=None, Z=None):

            """traverse move, laser off"""

            self.do_motion(X, Y, Z, self.traverse_ivl_native)
            self.emit('Qm')

        @code(require_any='XYZ')
        def G1(self, X=None, Y=None, Z=None, F=None):

            """linear move"""

            if F is not None:
                # N.B., F is units per MINUTE.
                ivl = self.native_ivl(float(F) / 60, self.distance_units)
                self.feed_ivl_native = ivl
            d = self.do_motion(X, Y, Z, self.feed_ivl_native)
            if self.pulse_mode == PulseMode.distance:
                pd = int(round(d / self.pulse_distance_usteps))
                self.emit('pd=%d' % pd)
            self.emit('Qc')

        @group_prepare
        def prepare_motion(self, mode, new_mode, settings, new_settings):

            # pp 20-21:
            #
            #    "If an axis word-using G-code from group 1 is implicitly
            #    in effect on a line (by having been activated on an
            #    earlier line), and a group 0 G-code that uses axis words
            #    appears on the line, the activity of the group 1 G-code
            #    is suspended for that line. The axis word-using G-codes
            #    from group 0 are G10, G28, G30, and G92."
            #
            # We only implement G28.

            if self.line_already_used_axes:
                return new_mode
        
            # if X, Y, or Z is coded on this line, do an implicit motion.
            X = new_settings.get('X')
            Y = new_settings.get('Y')
            Z = new_settings.get('Z')
            if all(axis is None for axis in (X, Y, Z)):
                return new_mode

            return new_mode or mode

        @group_finish
        def finish_motion(self, mode, new_mode, settings, new_settings):
            # Clear axis codes so next line won't use them.
            settings['X'] = None
            settings['Y'] = None
            settings['Z'] = None
        

    @code(nonmodal_group='dwell')
    def G4(self, P):

        """dwell"""

        mt = self.secs_to_ticks(P)
        self.emit('mt=%d' % mt, 'Qd')

    @code(modal_group='plane selection')
    def G17(self):

        """select XY plane"""

        # (No-op.)

    with modal_group('units'):

        @code
        def G20(self):

            """distances are in inches"""

            if self.distance_units == DistanceUnits.mm:
                self.x_pos.pos_units /= 25.4
                self.y_pos.pos_units /= 25.4
                self.z_pos.pos_units /= 25.4
            self.distance_units = DistanceUnits.inch

        @code
        def G21(self):

            """distances are in millimeters"""

            if self.distance_units == DistanceUnits.inch:
                self.x_pos.pos_units *= 25.4
                self.y_pos.pos_units *= 25.4
                self.z_pos.pos_units *= 25.4
            self.distance_units = DistanceUnits.mm

    @code(nonmodal_group='home')
    def G28(self):

        """home carriage"""

        self.line_already_used_axes = True
        self.abs_position_known = True
        self.x_pos.reset()
        self.y_pos.reset()
        self.z_pos.reset()
        self.emit('Qh', 'W')

    with modal_group('distance mode'):

        @code
        def G90(self):

            """distances are absolute positions"""

            self.distance_mode = DistanceMode.absolute

        @code
        def G91(self):

            """distances are relative positions"""

            self.distance_mode = DistanceMode.relative

    with nonmodal_group('stop'):

        @code
        def M0(self):

            """stop program"""

            return 'pause program'

        @code
        def M1(self):

            """optional stop program"""

            return 'optional pause program'

        @code
        def M2(self):

            """program end"""

            return 'end program'

    @code(modal_group='tool change')
    def M6(self, T):

        """select the laser"""

        ls = self.get_enum('M6', 'T', LaserSelect, T)
        if ls is not None:
            self.laser_select = ls
            self.emit('ls=%s' % ls)

    with modal_group('motors'):

        @code
        def M17(self):

            """enable stepper motors"""

            self.emit('Ex', 'Ey', 'Ez')

        @code
        def M18(self):

            """disable stepper motors"""

            self.emit('Dx', 'Dy', 'Dz')

    with modal_group('low voltage'):

        @code
        def M80(self):

            """enable low voltage power"""

            self.emit('El')

        @code
        def M81(self):

            """disable low voltage power"""

            self.emit('Dl')

    @code(modal_group='laser power')
    def M100(self, P):

        """set laser power level, P=0(off) to 1"""

        self.laser_power = P
        lp = max(0, min(4095, int(4095 * P)))
        self.emit('lp=%d' % lp)

    @code(modal_group='laser pulse width')
    def M101(self, P):
        pw = self.secs_to_ticks(P)
        self.emit('pw=%d' % pw)

    with modal_group('high voltage'):

        @code
        def M102(self):

            """enable high voltage power"""

            self.emit('Eh')

        @code
        def M103(self):

            """disable high voltage power"""

            self.emit('Dh')

    with modal_group('water'):

        @code
        def M104(self):

            """enable water cooling"""

            self.emit('Ew')

        @code
        def M105(self):

            """disable water cooling"""

            self.emit('Dw')

    with modal_group('air'):

        @code
        def M106(self):

            """enable air assist"""

            self.emit('Ea')

        @code
        def M107(self):

            """disable air assist"""

            self.emit('Da')

    with modal_group('laser pulse mode'):

        @code
        def M108(self):

            """set laser pulse mode to continuous fire"""

            self.emit('pm=c')
            self.pulse_mode = PulseMode.continuous

        @code
        def M109(self, S):

            """set laser pulse mode to timed"""

            self.pulse_mode = PulseMode.timed
            pi = self.secs_to_ticks(S)
            self.emit('pm=t', 'pi=%d' % pi)

        @code
        def M110(self, S):

            """set laser pulsed mode to distance"""

            self.pulse_mode = PulseMode.distance
            self.pulse_distance = S
            self.pulse_distance_usteps = (
                 self.x_pos.units_to_usteps(S,
                                            self.distance_units,
                                            integer=False))
            self.emit('pm=d')

        @code
        def M111(self):

            """set laser pulse mode to off"""

            self.emit('pm=o')
            self.pulse_mode = PulseMode.off

    @code(nonmodal_group='emergency stop')
    def M112(self):

        """emergency stop"""

        self.emit('S')
        return 'emergency stop'

    with modal_group('illumination'):

        @code
        def M113(self, P):

            """set illumination level"""

            il = max(0, min(127, int(P * 127)))
            self.emit('il=%d' % il)

        @code
        def M114(self, P):
            ia = self.get_enum('M114', 'P', Animation, P)
            if ia is not None:
                self.animation = ia
                self.emit('ia=%s' % ia)

    # #  #    #    #     #      #       #      #     #    #   #  # #

    def do_motion(self, X, Y, Z, ivl):
        xd = self.update_pos(self.x_pos, X)
        yd = self.update_pos(self.y_pos, Y)
        zd = self.update_pos(self.z_pos, Z)
        d = sqrt(xd**2 + yd**2 + zd**2)
        mt = d * ivl
        self.emit('xd=%+d' % xd,
                  'yd=%+d' % yd,
                  'zd=%+d' % zd,
                  'mt=%d' % mt)
        return d

    def update_pos(self, pos, amount):
        if amount is None:
            return 0
        if self.distance_mode == DistanceMode.absolute:
            if not self.abs_position_known:
                msg = "can't use absolute position; current position unknown."
                raise GCodeException(msg)
            pos.pos_units = amount
        else:
            pos.pos_units += amount
        prev_usteps = pos.pos_usteps
        new_usteps = pos.update_pos_usteps(self.distance_units)
        return new_usteps - prev_usteps

    def get_enum(self, code, sub_code, enum, number):
        value = ApproximateNumber.getitem(enum._map, number)
        if value is None:
            msg = '%s: %s must be ' % (code, sub_code)
            msg += ', '.join('%s=%s' % (n, enum.get_name(enum._map[n]))
                             for n in enum._map)
            raise GCodeException(msg)
        return value

    def secs_to_ticks(self, secs, integer=True):
        ticks = secs * F_CPU
        if integer:
            ticks = int(round(ticks))
        return ticks

    def native_ivl(self, rate, units=DistanceUnits.mm):
        # An "ivl," or interval, is inverse velocity: CPU ticks per
        # microstep.  It is a float.
        ivl = F_CPU / (rate * X_USTEPS_PER_INCH)
        if units == DistanceUnits.mm:
            ivl *= 25.4
        return ivl

    def emit(self, *cmds):
        for cmd in cmds:
            print cmd
