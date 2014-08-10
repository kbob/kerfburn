# Low Voltage Power Supply

def_output_pin    (PB6,     'Low Voltage Enable',  enabled=low)
def_input_pin     (PA0,     'Low Voltage Ready',   ready=high)


# Power Relays

def_output_pin    (PH5,     'High Voltage Enable', enabled=high)
def_output_pin    (PH6,     'Air Pump Enable',     enabled=high)
def_output_pin    (PB4,     'Water Pump Enable',   enabled=high)


# Lasers

def_timer         ('4',     'Laser Pulse')
def_timer_pin     (OC4A,    'Main Laser Pulse',    on=high)
def_timer_pin     (OC4B,    'Visible Laser Pulse', on=high)
def_timer_pin     (OC4C,    'Laser Watchdog')


# X Axis Motor

def_output_pin    (PD7,     'X Motor Enable',      enabled=low)
def_output_pin    (PF1,     'X Motor Direction',   positive=low)
def_timer_pin     (OC3B,    'X Motor Step',        on=high)
def_timer_pin     (OC3A,    'X Watchdog')


# Y Axis Motor

def_output_pin    (PF2,     'Y Motor Enable',      enabled=low)
def_output_pin    (PF7,     'Y Motor Direction',   positive=high)
def_timer_pin     (OC1A,    'Y Motor Step',        on=high)
def_timer_pin     (OC1B,    'Y Watchdog')


# Z Axis Motor

def_output_pin    (PK0,     'Z Motor Enable',      enabled=low)
def_output_pin    (PL1,     'Z Motor Direction',   positive=low)
def_timer_pin     (OC5A,    'Z Motor Step',        on=high)
def_timer_pin     (OC5B,    'Z Watchdog')


# Limit Switches

def_input_pin     (PE5,     'X Min Switch',        reached=high, pull_up=True)
def_input_pin     (PJ1,     'Y Min Switch',        reached=high, pull_up=True)
# def_input_pin   (PD3,     'Z Min Switch',        reached=high, pull_up=True)
# def_input_pin   (PD2,     'Z Max Switch',        reached=high, pull_up=True)


# Misc. switches

def_interrupt_pin (PCINT19, 'Emergency Stop',      stopped=low, pull_up=True)
def_interrupt_pin (PCINT20, 'Lid',                 open=high,    pull_up=True)


# LED (on Azteeg board)

def_output_pin    (PB7,      'LED',                 on=high)


# LEDs (on Gantry)

def_output_pin    (_SS,     'SPI SS')
def_output_pin    (SCK,     'SPI SCK')
def_output_pin    (MOSI,    'SPI MOSI')
def_output_pin    (MISO,    'SPI MISO')


# i2c, used for main laser power

def_input_pin     (SCL,     'I2C_CLOCK',            pull_up=True)
def_input_pin     (SDA,     'I2C_DATA',             pull_up=True)
