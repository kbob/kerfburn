# Configuration

Random thoughts

I need a way to declare hardware features and to declare
which pins and registers do what.

Something like this.

    X = axis {
      motor = {
        enable:    =  D7
        direction  =  F1
        step:      =  E4
        pulse:     =  3B
        step_size  =   0.008 in * 25.4 mm/in
        microsteps =  16
        length     = 530 mm
        home       = min
      }
      min = switch {
        pin        =  E5
        active     = low
      }
      max = undefined
    }
    lid = switch {
      pin          = ??
      open         = high
    }        
    LEDs = SPI {
      select       = 
      clock        = B1
      data         = B2
    }
    relays = {
      high_voltage = {
        pin        = H5
        active     = high
      }
      water_pump = {
        pin        = B4
        active     = high
      }
      air_pump = {
        pin        = H6
        active     = high
      }
    }
    visible_laser = {
      pulse_pin    = H4
      pulse_register = 4B
    }
