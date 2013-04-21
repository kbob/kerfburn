# Low Voltage Power Supply

def_output_pin (PB6,  'Low Voltage Enable', enabled=low)
def_input_pin  (PA0,  'Low Voltage Ready',  ready=high)


# Power Relays

def_output_pin(PH5,  'High Voltage Enable', enabled=high)
def_output_pin(PH6,  'Air Pump Enable',     enabled=high)
def_output_pin(PB4,  'Water Pump Enable',   enabled=high)


# Lasers

def_timer     ('4',  'Laser Pulse');
def_timer_pin(OC4A,  'Main Laser Pulse',       on=high);
def_timer_pin(OC4B,  'Visible Laser Pulse',    on=high);


# X Axis Motor

def_output_pin(PD7,  'X Motor Enable',      enabled=low)
def_output_pin(PF1,  'X Motor Direction',   positive=low)
def_timer_pin(OC3B,  'X Motor Step')
# step_size  =   0.08 * 20 / 200 * 25.4, # mm
# microsteps =  16,
# length     = 530        # mm


# Y Axis Motor

def_output_pin(PF2,  'Y Motor Enable',      enabled=low)
def_output_pin(PF7,  'Y Motor Direction',   positive=low)
def_timer_pin(OC1A,  'Y Motor Step');
# step_size  =   0.08 * 20 / 200 * 25.4, # mm
# microsteps =  16,
# length     = 280        # mm


# Z Axis Motor

def_output_pin(PK0,  'Z Motor Enable',      enabled=low)
def_output_pin(PL1,  'Z Motor Direction',   positive=low)
def_timer_pin(OC5A,  'Z Motor Step');
# step_size  =   0.08 * 20 / 200 * 25.4, # mm
# microsteps =  16,
# length     = 280        # mm


# Limit Switches

def_input_pin(PE5,  'X Min Switch',         reached=high, pull_up=True)
def_input_pin(PJ1,  'Y Min Switch',         reached=high, pull_up=True)
def_input_pin(PD3,  'Z Min Switch',         reached=high, pull_up=True)
def_input_pin(PD2,  'Z Max Switch',         reached=high, pull_up=True)


# Misc. switches

def_input_pin (PA3,  'Emergency Stop',      stopped=low,  pull_up=True)
def_input_pin (PC5,  'Lid',                 open=low,     pull_up=True)


# LED (on Azteeg board)

def_output_pin(PB7,  'LED',                 on=high)


# LEDs (on Gantry)

def_output_pin(_SS,  'SPI SS')
def_output_pin(SCK,  'SPI SCK')
def_output_pin(MISO, 'SPI MISO')
def_output_pin(MOSI, 'SPI MOSI')
