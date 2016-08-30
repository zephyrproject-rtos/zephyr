Title: Power management demo

Description:

A sample implementation of a power manager app that uses the zephyr
power management infrastructure.

This app will cycle through the various power schemes at every call
to _sys_soc_suspend() hook function.
It will cycle through following policies
1. Low Power State (LPS) - puts the CPU in C2 state
2. Device suspend only - demonstrates hooks into kernel idle entry and exit
that can be used to only suspend devices without CPU or SOC PM operations.
3. Deep Sleep - demonstrates invoking suspend/resume handlers of devices to
save device states and switching to deep sleep state.
4. No-op - no operation and letting kernel do its idle

This application runs on quark se boards.

When started, the app waits for a toggle of GPIO pin 16.  GPIO Pin 16 is
DIO 8 in arduino_101 and DIO 4 in quark_se_c1000_devboard.

Toggle the GPIO pin in the following sequence:-
	1) Start connected to GND during power on or reset.
	2) Disconnect from GND and connect to 3.3V
	3) Disconnect from 3.3V. Test should start now.

Known issues:
The test uses RTC to wake it up from deep sleep.  After several iterations,
the RTC might fail to fire an alarm or the alarm is missed.  It appears to be
a timing issue with using RTC for his purpose. If this happens, the app would
be unable to wake up from deep sleep and would need a reset.
--------------------------------------------------------------------------------

Building and Running Project:

This application is architecture and SoC specific.  It is written for x86
architecture and uses features specific to quark_se platforms.

This is a microkernel only project since the zephyr power management
infrastructure currently is only microkernel idle based. In future, when
nanokernel idle is supported, a separate nanokernel app would be created.

    make BOARD=<quark_se board>

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
PM test started


Low power state policy entry!

Low power state policy exit!
Total Elapsed From Suspend To Resume = 163869 RTC Cycles
Device suspend only policy entry!

Device suspend only policy exit!
Total Elapsed From Suspend To Resume = 163868 RTC Cycles


Deep sleep policy entry!
Wake from Deep Sleep!

Deep sleep policy exit!
Total Elapsed From Suspend To Resume = 163986 RTC Cycles
Deep Sleep wake up event handler

...
