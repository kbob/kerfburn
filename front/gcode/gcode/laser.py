"""G-Code Machine Protocol for laser cutter"""

import sys

from gcode.core import ApproximateNumber, Executor, GCodeException
from gcode.core import code, modal_group, nonmodal_group
from gcode.parser import parse_comment


DEFAULT_FEED_RATE = 20          # mm/sec

class UnitsMode:
    mm = 0
    inch = 1

class DistanceMode:
    absolute = 0
    relative = 1


class LaserExecutor(Executor):

    def __init__(self):
        self.message = ''
        self.feed_rate = DEFAULT_FEED_RATE
        self.spindle_speed = None
        self.current_tool = None
        self.next_tool = None
        self.laser_select = None
        self.coolant_on = [False, False]
        self.length_units = UnitsMode.mm
        self.distance_mode = DistanceMode.absolute

    def initial_settings(self):
        return {
            'F': 25,
            'P': None,
            'S': None,
            'T': None,
            'X': None,
            'Y': None,
            'Z': None,
            }

    @property
    def order_of_execution(self):
        return (
            self.exec_comment,
            'low voltage',
            'illumination',
            'tool change',
            'spindle',
            'water',
            'air',
            'high voltage',
            'laser power',
            'laser pulse mode',
            'laser pulse width',
            'motors',
            'dwell',
            'units',
            'distance mode',
            'home',
            'motion',
            'stop',
            )

    def exec_comment(self, settings, new_settings, pline):
        if pline.comment:
            hdr, rest = parse_comment(pline.source.pos, pline.comment)
            if hdr == 'MSG':
                print >>sys.stderr, 'MSG', rest
    
    # #  #    #    #     #      #       #      #     #    #   #  # #

    with modal_group('motion'):

        @code(require_any='XYZ')
        def G0(self, X, Y, Z):
            "traverse move"

        @code(require_any='XYZ')
        def G1(self, X, Y, Z, F):
            """linear motion"""

    @code(nonmodal_group='dwell')
    def G4(self, P):
        "dwell"

    with modal_group('units'):

        @code
        def G20(self):
            """distances are in inches"""
            self.length_units = UnitsMode.inch
            # XXX correct all positions/speeds

        @code
        def G21(self):
            """distances are in millimeters"""
            self.length_units = UnitsMode.mm
            # XXX correct all positions/speeds

    @code(nonmodal_group='home')
    def G28(self):
        """home carriage"""
        # pp 20-21:
        #
        #    "If an axis word-using G-code from group 1 is implicitly
        #    in effect on a line (by having been activated on an
        #    earlier line), and a group 0 G-code that uses axis words
        #    appears on the line, the activity of the group 1 G-code
        #    is suspended for that line. The axis word-using G-codes
        #    from group 0 are G10, G28, G30, and G92."
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
            return 'pause program'

        @code
        def M2(self):
            """program end"""
            return 'end program'

    with modal_group('spindle'):

        @code
        def M3(self, S):
            """enable the current laser"""
            self.laser_enabled = True

        @code
        def M4(self, S):
            """enable the current laser"""
            self.laser_enabled = True

        @code
        def M5(self):
            """disable the laser"""
            self.laser_enabled = False

    @code(modal_group='tool change')
    def M6(self, T):
        """select the laser"""
        if ApproximateNumber(0) == T:
            ls = 'n'
        elif ApproximateNumber(1) == T:
            ls = 'm'
        elif ApproximateNumber(2) == T:
            ls = 'v'
        else:
            msg = 'M6: T must be 0=off, 1=main laser, or 2=visible laser'
            raise GCodeException(msg)
        self.emit('ls=%s' % ls)

    with modal_group('motors'):

        @code
        def M17(self):
            pass

        @code
        def M18(self):
            pass

    with modal_group('low voltage'):

        @code
        def M80(self):
            pass

        @code
        def M81(self):
            pass

    @code(modal_group='laser power')
    def M100(self, P):
        pass

    @code(modal_group='laser pulse width')
    def M101(self, P):
        pass

    with modal_group('high voltage'):

        @code
        def M102(self):
            pass

        @code
        def M103(self):
            pass

    with modal_group('water'):

        @code
        def M104(self):
            pass

        @code
        def M105(self):
            pass

    with modal_group('air'):

        @code
        def M106(self):
            pass

        @code
        def M107(self):
            pass

    with modal_group('laser pulse mode'):

        @code
        def M108(self):
            pass

        @code
        def M109(self):
            pass

        @code
        def M110(self):
            pass

        @code
        def M111(self):
            pass

    @code(nonmodal_group='stop')
    def M112(self):
        pass

    with modal_group('illumination'):

        @code
        def M113(self, P):
            pass

        @code
        def M114(self, P):
            pass

    # #  #    #    #     #      #       #      #     #    #   #  # #

    def emit(self, *cmds):
        for cmd in cmds:
            print cmd
