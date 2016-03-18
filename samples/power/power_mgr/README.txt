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
3. No-op - no operation and letting kernel do its idle

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

Low power state policy entry!

Low power state policy exit!
Total Elapsed From Suspend To Resume = 163838 RTC Cycles
Device suspend only policy entry!

Device suspend only policy exit!
Total Elapsed From Suspend To Resume = 163838 RTC Cycles

...
