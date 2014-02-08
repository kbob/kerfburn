import abc

from gcode.core import code, modal_group, GCodeException, CheapEnum

# Things to put in this file:
#
#   AxisPosition
#   PositionMode (abs/rel)
#   DistanceUnits (mm/inch)
#   MotionMixIn
#   implicit motion

class DistanceMode:
    absolute = 0
    relative = 1

class DistanceUnits:
    mm = 0
    inch = 1


# class MotionMixIn(object):
#
#     __metaclass__ = abc.ABCMeta
#
#     @property
#     def initial_settings(self):
#         return {}

