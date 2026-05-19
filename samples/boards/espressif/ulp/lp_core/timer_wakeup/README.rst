.. zephyr:code-sample:: lp-timer-wakeup
   :name: LP Timer Wakeup

   LP Core periodically wakes HP Core from deep sleep using LP timer.

Overview
********

This sample demonstrates the LP core using the LP timer to periodically
wake the HP core from deep sleep. The LP core operates in a one-shot
model: it wakes up, signals the HP core, re-arms the LP timer using
the sleep duration configured by the HP core via shared memory, and
halts until the next timer alarm.

The HP core boots, prints its wake count, and enters deep sleep via
``sys_poweroff()``. A ``RTC_DATA_ATTR`` variable tracks the number of
wakeups across deep sleep cycles.

The sleep duration is configured via
:kconfig:option:`CONFIG_ESP32_ULP_LP_CORE_LP_TIMER_SLEEP_DURATION_US`
(default: 2000000 us = 2 seconds).

How it works
************

1. HP core loads LP core binary, configures LP timer wakeup source with
   the desired sleep duration, and enters deep sleep.
2. LP core wakes from LP timer alarm, calls
   ``ulp_lp_core_wakeup_main_processor()`` to wake the HP core.
3. LP core reads the pre-computed sleep duration (in ticks) from shared
   memory and re-arms the LP timer for the next cycle.
4. LP core calls ``ulp_lp_core_halt()`` to enter low-power sleep.
5. On the next LP timer alarm, the LP core resets and repeats from step 2.

Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/timer_wakeup
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/timer_wakeup
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

    First boot, LP core will wake us every 2000 ms
    HP core entering deep sleep...

After wakeup:

.. code-block:: console

    Woke up from LP core (LP timer)! Wake count: 1
    HP core entering deep sleep...
