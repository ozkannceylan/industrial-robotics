# RobotMode: discrete operational state of the robot.
#
# Constants are uppercase by convention. The "= <value>" makes them
# compile-time constants (no field, just named integer values).
uint8 MODE_IDLE    = 0
uint8 MODE_RUNNING = 1
uint8 MODE_FAULT   = 2
uint8 MODE_ESTOP   = 3

# A single field carrying the current mode.
# Use one of the constants above as its value.
uint8 mode
