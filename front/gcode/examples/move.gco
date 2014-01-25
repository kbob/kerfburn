%
(Home, then move to X=100, Y=100)

G21 (units are millimeters)
G90 (use absolute coordinates)
M80 (enable low voltage power) M17 (enable steppers) G28 (home)
G0 X+100 Y+100
M18 M81 (disable steppers, power down)
%
