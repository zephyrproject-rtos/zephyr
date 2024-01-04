Setpoints Clock Test
#######################

This test is designed to verify the platform can switch setpoints between two
different frequencies on a given clock. The setpoints for this test should keep
the clock frequency for the system timer and console output unchanged, but
change the frequency of the clock given by the "clocks" property of the
zephyr,user node.


The test has the following phases:
- select CLOCK_SETPOINT_RUN
- query the clock frequency given by clocks property of the zephyr,user node
- verify this frequency matches the run-freq property of the zephyr,user node
- select CLOCK_SETPOINT_IDLE
- query the clock frequency given by clocks property of the zephyr,user node
- verify this frequency matches the idle-freq property of the zephyr,user node
