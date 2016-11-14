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
3. No-op - no operation and letting kernel do its idle

This application uses Intel Quark SE Microcontroller C1000 board for
the demo. It demonstrates power operations on the x86 and ARC cores in
the board.
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

***Sample output on x86 core***

Power Management Demo on x86

Application main thread

Low power state entry!

Low power state exit!
Total Elapsed From Suspend To Resume = 131073 RTC Cycles
Wake up event handler

Application main thread

Deep sleep entry!
Wake from Deep Sleep!

Deep sleep exit!
Total Elapsed From Suspend To Resume = 291542 RTC Cycles
Wake up event handler

Application main thread

No PM operations done

Application main thread

Low power state entry!

Low power state exit!

...

***Sample output on ARC core***

Power Management Demo on arc

Application main thread

Low power state entry!

Low power state exit!
Total Elapsed From Suspend To Resume = 131073 RTC Cycles
Wake up event handler

Application main thread

No PM operations done

Application main thread

Low power state entry!

Low power state exit!

...
