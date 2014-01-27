%
(Cut a rectangle.)

G21                             (units are millimeters)
G91                             (use relative coordinates)

G0 X20 Y155                     (move out)
G1 X+135 F[60*40]               (cut bottom - feed rate 40 mm/sec)
Y+70                            (cut right side)
X-130                           (cut top)
Y-70                            (cut left side)

%
