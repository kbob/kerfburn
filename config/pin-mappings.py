# Low Voltage Power Supply

def_output_pin (PB6,  'Low Voltage Enable', enabled=low)
def_input_pin  (PA0,  'Low Voltage Ready',  ready=high)

# Power Relays

def_output_pin(PH5,  'High Voltage Enable', enabled=high)
def_output_pin(PH6,  'Air Pump Enable',     enabled=high)
def_output_pin(PB4,  'Water Pump Enable',   enabled=high)

# Main Laser

def_output_pin(PE3,  'Main Laser Fire',     on=high);
def_timer_pin(OC3A,  'Main Laser Pulse');

# Visible Laser

def_output_pin(PH4,  'Visible Laser Fire',  on=high);
def_timer_pin(OC4B,  'Visible Laser Pulse');

# X Axis Motor

def_output_pin(PD7,  'X Motor Enable',      enabled=low)
def_output_pin(PF1,  'X Motor Direction',   positive=low)
def_timer_pin(OC3A,  'X Motor Step')
# step_size  =   0.08 * 20 / 200 * 25.4, # mm
# microsteps =  16,
# length     = 530        # mm

# Y Axis Motor

def_output_pin(PK0,  'Y Motor Enable',      enabled=low)
def_output_pin(PL1,  'Y Motor Direction',   positive=low)
def_timer_pin(OC5A,  'Y Motor Step');
# step_size  =   0.08 * 20 / 200 * 25.4, # mm
# microsteps =  16,
# length     = 280        # mm

# Z Axis Motor

# def_output_pin(P??,  'Z Motor Enable',      enabled=low)
# def_output_pin(P??,  'Z Motor Direction',   positive=low)
# def_timer_pin(OC??,  'Z Motor Step');
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

# LED (on Azteeg)

def_output_pin(PB7,  'LED',                 on=high)

# LEDs (on Gantry)

def_output_pin(_SS,  'SPI SS')
def_output_pin(SCK,  'SPI SCK')
def_output_pin(MISO, 'SPI MISO')
def_output_pin(MOSI, 'SPI MOSI')
