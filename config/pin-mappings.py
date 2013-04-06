# Low Voltage Power Supply

def_output_pin (PB6, 'Low Voltage Enable', enabled=low)
def_input_pin  (PA0, 'Low Voltage Ready',  ready=high)

# Power Relays

def_output_pin(PH5, 'High Voltage Enable', enabled=high)
def_output_pin(PB4, 'Water Pump Enable',   enabled=high)
def_output_pin(PH6, 'Air Pump Enable',     enabled=high)

# X Axis Motor

def_output_pin(PD7, 'X Motor Enable',      enabled=low)
def_output_pin(PF1, 'X Direction',         positive=low)
# def_timer_output(OCR3A, 

# Misc. switches

def_input_pin (PC5, 'Lid',                 open=low, pull_up=True)

# LEDs

def_output_pin(_SS, 'SPI SS')
def_output_pin(SCK, 'SPI SCK')
def_output_pin(MISO, 'SPI MISO')
def_output_pin(MOSI, 'SPI MOSI')

# low_voltage = (
#    enable  = output(pin=B6, enable=low),
#    ready   = pullup(pin=Z0, ready=low))
                  
# high_voltage = (
#     output(pin=H5, enable=high),
#     output(pin=B4, enable=high),
#     output(pin=H6, enable=high))

# X = axis (
#   motor = (
#     enable     =  D7,
#     direction  =  F1,
#     step       =  E4,
#     pulse      = OCR3B,
#     step_size  =   0.08 * 20 / 200 * 25.4, # mm
#     microsteps =  16,
#     length     = 530        # mm
#   ),
#   min = pullup (
#     pin        =  D3,
#     reached    = low
#   ),
#   home         = min
# )

# Y = axis (
#   motor = (
#     enable     =  K0,
#     direction  =  L1,
#     step       =  L3,
#     pulse      =  OCR0A,
#     step_size  =   0.08 * 20 / 200 * 25.4, # mm
#     microsteps =  16,
#     length     = 280        # mm
#   ),
#   max = pullup (
#     pin        =  E5,
#     reached    = low
#   ),
#   home         = min
# )

# lid = pullup(pin=D32, open=high)


# main_laser = laser (
#     pulse
#     pulse_pin = Z0,
#     level     = Z0,
#     pulse     = OCR

# def visible_laser():
#   pulse_pin    = H4,
#   pulse_register = OCR4B

# LEDs = SPI (
#   clock        = SCK,
#   data         = MOSI
# )
