"""G-Code Machine Protocol for laser cutter"""

from gcode import core
from gcode.core import code, modal_group, nonmodal_group
from gcode.parser import parse_comment


DEFAULT_FEED_RATE = 20          # mm/sec

class UnitsMode:
    mm = 0
    inch = 1

class DistanceMode:
    absolute = 0
    relative = 1


class LaserExecutor(core.Executor):

    def __init__(self):
        self.message = ''
        self.feed_rate = DEFAULT_FEED_RATE
        self.spindle_speed = 0
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
                print 'MSG', rest
    
    # #  #    #    #     #      #       #      #     #    #   #  # #

    with modal_group('motion'):

        @code(require_any='XYZ')
        def G0(self, X, Y, Z):
            pass

        @code(require_any='XYZ')
        def G1(self, X, Y, Z, F):
            pass

    @code(nonmodal_group='dwell')
    def G4(self, P):
        pass

    with modal_group('units'):

        @code
        def G20(self):
            self.length_units = UnitsMode.inch
            # XXX correct all positions/speeds

        @code
        def G21(self):
            self.length_units = UnitsMode.mm
            # XXX correct all positions/speeds

    @code(nonmodal_group='home')
    def G28(self):
        # pp 20-21:
        #
        #    "If an axis word-using G-code from group 1 is implicitly
        #    in effect on a line (by having been activated on an
        #    earlier line), and a group 0 G-code that uses axis words
        #    appears on the line, the activity of the group 1 G-code
        #    is suspended for that line. The axis word-using G-codes
        #    from group 0 are G10, G28, G30, and G92."
        pass

    with modal_group('distance mode'):

        @code
        def G90(self):
            pass

        @code
        def G91(self):
            pass

    with nonmodal_group('stop'):

        @code
        def M0(self):
            pass

        @code
        def M1(self):
            pass

        @code
        def M2(self):
            pass

    with modal_group('spindle'):

        @code
        def M3(self, S):
            pass

        @code
        def M4(self, S):
            pass

        @code
        def M5(self):
            pass

    @code(modal_group='tool change')
    def M6(self, T):
        pass

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

    
