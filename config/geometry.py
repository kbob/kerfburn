x_axis = Axis(
    # motor:   200 steps/rev
    # pulley:   20 teeth
    # belt:   0.08 in/tooth
    step_size  = 0.08 * 20 / 200 * 25.4,
    microsteps = 16,
    length     = 530,
    max_speed  = 100,
    )
    
y_axis = Axis(
    # motor:   200 steps/rev
    # pulley:   20 teeth
    # belt:   0.08 in/tooth
    step_size  = 0.08 * 20 / 200 * 25.4,
    microsteps = 16,
    length     = 280,
    max_speed  = 100,
    )
    
z_axis = Axis(
    # motor:             200 steps/rev
    # primary pulley:     48 teeth
    # belt:             0.08 in/tooth
    # secondary pulley:   20 teeth
    # screw pitch:      1/20 inch
    step_size  = 0.08 * 48 / 20 / 200 / 20 * 25.4,
    microsteps = 16,
    length     =  0,            # XXX No Z yet.
    max_speed  = 10,
    )

home = Position(x=min, y=min, speed=[100, 10, 5])
origin = Position(x=min, y=min)

