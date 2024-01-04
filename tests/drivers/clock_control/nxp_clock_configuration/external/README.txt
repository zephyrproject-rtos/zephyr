NXP External Clock Test
#######################

This test is designed to verify the platform can boot from the highest
frequency external crystal available on the part. The clock configuration for
this test should always be the highest frequency configuration supported by
the board.

The test itself is very simple- it queries the clock_control API for the
frequency of the clock subsystem given in clocks zephyr,user phandle,
and verifies that the frequency matches the clock-freq value given
in the zephyr,user node. Generally, the clock subsystem in the clocks
phandle should refer to the system core clock.
