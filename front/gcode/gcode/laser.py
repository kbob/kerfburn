"""G-Code Machine Protocol for laser cutter"""

from gcode import core
from gcode.core import code, modal_group

DEFAULT_FEED_RATE = 100         # mm/sec

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

    def initial_modes(self):
        return {
            'F': 25,
            'P': None,
            'S': None,
            'T': None,
            'X': None,
            'Y': None,
            'Z': None,
            }

    # def order_of_execution(self):
    #     return ()
    #     # return (
    #     #         self.exec_comment,
    #     #         self.exec_set_feed_rate,
    #     #         self.exec_set_spindle_speed,
    #     #         self.exec_select_tool,
    #     #         self.exec_change_tool,
    #     #         self.exec_spindle_on_off,
    #     #         self.exec_coolant_on_off,
    #     #         self.exec_dwell,
    #     #         self.exec_set_length_units,
    #     #         self.exec_set_distance_mode,
    #     #         self.exec_home,
    #     #         self.exec_motion,
    #     #         self.exec_stop,
    #     #         )

    def execute(self, modes, pline):
        #  1. comment (do nothing)
        #  2. set feed rate
        #  3. set spindle speed
        #  4. select tool
        #  5. change tool
        #  6. spindle on off
        #  7. coolant on off
        #  8. dwell
        #  9. set length units
        # 10. set distance mode
        # 11. home
        # 12. motion
        # 13. stop
        pass


    with modal_group('motion'):

        @code(require_any='XYZ')
        def G0(self, X, Y, Z):
            pass

        @code(require_any='XYZ')
        def G1(self, X, Y, Z, F):
            pass

    @code(modal_group='dwell')
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

    @code(modal=False)
    def G28(self):
        pass

    with modal_group('distance mode'):

        @code
        def G90(self):
            pass

        @code
        def G91(self):
            pass

    with modal_group('stopping'):

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

    @code
    def M100(self, P):
        pass

    @code
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

    with modal_group('pulse mode'):

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

    @code(modal_group='stopping')
    def M112(self):
        pass

    with modal_group('illumination'):

        def M113(self, P):
            pass

        def M114(self, P):
            pass

    
