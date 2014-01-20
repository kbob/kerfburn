# Arcs are not implemented.  This just illustrates how to add
# capabilities to an Executor through a mixin.

from gcode.core import code, modal_group

class ArcMixin(object):

    with modal_group('motion'):

        @code
        def G2(self, X, Y, Z, A, B, C, I, J, K, R):
            pass

        @code
        def G3(self, X, Y, Z, A, B, C, I, J, K, R):
            pass


if __name__ == '__main__':

    import gcode.core

    class MyExecutor(gcode.core.Executor, ArcMixin):

        @code(modal_group='motion')
        def G0(self, X, Y, Z):
            pass

        def initial_modes(self):
            pass

        def order_of_execution(self):
            pass

    mex = MyExecutor()
    print mex.dialect

