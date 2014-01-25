(Initialization)

M80                             (enable low voltage power)
M17                             (enable steppers)

M106                            (start water cooling)
M102                            (enable high voltage power)
M104                            (enable air assist)

M6 T2                           (select visible laser)
M109 S0.010                     (set timed pulse mode, 10 msec apart)
M101 P0.005                     (set pulse width, 5 msec.)

G28
