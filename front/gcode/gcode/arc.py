# Arcs are implemented as a mix-in class.  Inherit from Executor and
# XYArcMixIn to create a class that subdivides args into G1 moves.

from math import sin, cos, atan2, hypot, pi

from gcode.core import code, modal_group, GCodeException
from gcode.motion import DistanceMode, DistanceUnits


MM_PER_ARC_SEGMENT = 0.5
N_ARC_CORRECTION = 25

class XYArcMixin(object):

    @property
    def initial_settings(self):
        return {
            'X': None,
            'Y': None,
            'I': None,
            'J': None,
            'R': None,
            'F': None,
            }

    with modal_group('motion'):

        @code(require_any='XY')
        def G2(self, X=None, Y=None, I=None, J=None, R=None, F=None):
            """clockwise arc"""
            return self.do_arc('G2', -1, X, Y, I, J, R, F)

        @code(require_any='XY')
        def G3(self, X=None, Y=None, I=None, J=None, R=None, F=None):
            """counterclockwise arc"""
            return self.do_arc('G3', +1, X, Y, I, J, R, F)

    def do_arc(self, code_name, direction, X, Y, I, J, R, F):
        has_center = any(c is not None for c in (I, J))
        has_radius = R is not None
        if has_center and has_radius:
            msg = "%s: can't specify both radius (R) and center {I, J}"
            msg %= code_name
            raise GCodeException(msg)
        elif not has_center and not has_radius:
            msg = '%s: must specify either radius (R) or center (I, J)'
            msg %= code_name
            raise GCodeException(msg)

        if has_radius:
            # Construct isosceles triangle between arc end points and center.
            # Calculate center, offset, theta.
            raise NotImplementedError()
        else:                   # has_center
            curr = [self.x_pos.pos_units, self.y_pos.pos_units]
            offset = [0, 0]
            if I is not None:
                offset[0] = I
            if J is not None:
                offset[1] = J
            dest = curr[:]
            center = [curr[i] + offset[i] for i in (0, 1)]
            if X is not None:
                if self.distance_mode == DistanceMode.absolute:
                    dest[0] = X
                else:
                    dest[0] += X
            if Y is not None:
                if self.distance_mode == DistanceMode.absolute:
                    dest[1] = Y
                else:
                    dest[1] += Y
            r0 = [-offset[i] for i in (0, 1)]
            r1 = [dest[i] - center[i] for i in (0, 1)]
            self.check_on_arc(dest, r0, r1)
            theta = atan2(r0[0] * r1[1] - r0[1] * r1[0],
                          r0[0] * r1[0] + r0[1] * r1[1])
                          
            # dir positive -> 0 <= theta < 2pi
            # dir negative -> -2pi < theta <= 0
            assert direction in (-1, +1)
            if direction * theta < 0:
                theta += direction * 2 * pi
            distance = abs(theta) * hypot(*r1)
            if self.distance_units == DistanceUnits.inch:
                distance *= 25.4
            segment_count = max(1, int(distance / MM_PER_ARC_SEGMENT))
            theta_per_segment = theta / segment_count
            # approx: http://en.wikipedia.org/wiki/Small-angle_approximation
            sin_tps = theta_per_segment
            cos_tps = 1 - theta_per_segment**2
            r = r0
            for i in range(segment_count - 1):
                if (i + 1) % N_ARC_CORRECTION:
                    r = [r[0] * cos_tps - r[1] * sin_tps,
                         r[0] * sin_tps + r[1] * cos_tps]
                else:
                    cos_Ti = cos(i * theta_per_segment)
                    sin_Ti = sin(i * theta_per_segment)
                    r = [-offset[0] * cos_Ti + offset[1] * sin_Ti,
                         -offset[0] * sin_Ti - offset[1] * cos_Ti]
                prev = curr[:]
                curr = [center[j] + r[j] for j in (0, 1)]
                if self.distance_mode == DistanceMode.absolute:
                    step = curr
                else:
                    step = [curr[j] - prev[j] for j in (0, 1)]
                self.G1(X=step[0], Y=step[1], F=F)
            if self.distance_mode == DistanceMode.absolute:
                step = dest
            else:
                step = [dest[i] - center[i] - r[i] for i in (0, 1)]
            self.G1(X=step[0], Y=step[1], F=F)

    def check_on_arc(self, dest, r0, r1):
        l0 = hypot(*r0)
        l1 = hypot(*r1)
        max_err = 0.002 if self.distance_units == DistanceUnits.mm else 0.0002
        if abs(l1 - l0) > max_err:
            msg = 'destination (%g, %g) is not on arc' % (dest[0], dest[1])
            raise GCodeException(msg)


if __name__ == '__main__':

    import gcode.core

    class MyExecutor(gcode.core.Executor, XYArcMixin):

        @code(modal_group='motion')
        def G0(self, X, Y, Z):
            pass

        @property
        def initial_settings(self):
            return {}

        @property
        def order_of_execution(self):
            return ('motion',)

    mex = MyExecutor()
    print mex.dialect

