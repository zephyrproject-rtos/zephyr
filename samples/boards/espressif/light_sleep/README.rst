.. zephyr:code-sample:: esp32-light-sleep
   :name: Light Sleep

   Use light sleep mode on ESP32 to save power while preserving the state of the memory,
   CPU, and peripherals.

Overview
********

This example illustrates usage of light sleep mode. Unlike deep sleep mode, light sleep
preserves the state of memory, CPU, and peripherals.

Light sleep is entered automatically while the system is idle and power management
is enabled. In the current implementation, wake-up from light sleep is supported only
via an RTC timer, since execution resumes under control of the Zephyr scheduler and
the wake-up time is scheduled in advance.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/light_sleep
   :board: esp32_devkitc/esp32/procpu
   :goals: build flash
   :compact:

Sample Output
=============

The output below shows the system repeatedly entering light sleep while
calling ``k_sleep()``. The sample uses the minimum residency time configured
for the light sleep (standby) power state in the device tree, applies a timing
margin, and transitions the system into light sleep. The system wakes up via
the RTC timer and resumes execution under control of the Zephyr scheduler.

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-5178-gf042bd21bce7 ***
   Entering light sleep
   Returned from light sleep, reason: timer, t=218 ms, slept for 51 ms
   Entering light sleep
   Returned from light sleep, reason: timer, t=279 ms, slept for 51 ms
   Entering light sleep
   Returned from light sleep, reason: timer, t=340 ms, slept for 51 ms
   Entering light sleep
   Returned from light sleep, reason: timer, t=401 ms, slept for 51 ms
