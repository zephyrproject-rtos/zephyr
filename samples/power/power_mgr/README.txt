Title: Power management demo

Description:

A sample implementation of a power manager app that uses the zephyr
power management infrastructure.

This app will cycle through the various power schemes at every call
to _sys_soc_suspend() hook function.
It will cycle through following states
1. CPU Low Power State
2. Deep Sleep - demonstrates invoking suspend/resume handlers of devices to
save device states and switching to deep sleep state.
4. No-op - no operation and letting kernel do its idle

This application uses Intel Quark SE Microcontroller C1000 board for
the demo.

When started, the app waits for a toggle of GPIO pin 16.  GPIO Pin 16 is
DIO 8 in arduino_101 and DIO 4 in quark_se_c1000_devboard.

Toggle the GPIO pin in the following sequence:-
	1) Start connected to GND during power on or reset.
	2) Disconnect from GND and connect to 3.3V
	3) Disconnect from 3.3V. Test should start now.

--------------------------------------------------------------------------------

Building and Running Project:

    make BOARD=<board>

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

Power Management Demo
Toggle gpio pin 16 to start test
PM demo started


Low power state entry!

Low power state exit!
Total Elapsed From Suspend To Resume = 163869 RTC Cycles

Deep sleep entry!
Wake from Deep Sleep!

Deep sleep exit!
Total Elapsed From Suspend To Resume = 163986 RTC Cycles
Deep Sleep wake up event handler

...
