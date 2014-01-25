%
(Cut a rectangle.)

M80                             (enable low voltage power)
M17                             (enable steppers)

M106                            (start water cooling)
M102                            (enable high voltage power)
M104                            (enable air assist)

M6 T2                           (select visible laser)
M109 S0.010                     (set timed pulse mode, 10 msec apart)
M101 P0.005                     (set pulse width, 5 msec.)

G21                             (units are millimeters)
G91                             (use relative coordinates)

G28                             (home)
G0 X110 Y155                    (move out)
G1 X+135 F[60*40]                (cut bottom - default feed rate)
Y+70                            (cut right side)
X-130                           (cut top)
Y-70                            (cut left side)

M107                            (disable air assist)

G28                             (home)

M6 T0                           (select no laser)
M103                            (disable high voltage power)
G4 P10                          (dwell 10 seconds)
M105                            (stop water cooling)

M18                             (disable steppers)
M81                             (disable low voltage power)
%
