.. zephyr:code-sample:: lp-gpio-wakeup
   :name: LP GPIO Wakeup

   LP Core wakes HP Core from deep sleep on LP GPIO level change.

Overview
********

This sample demonstrates the LP core waking the HP core from deep sleep
when a low level is detected on an LP GPIO pin. The LP core uses the
Zephyr GPIO API to configure the pin and enable the wakeup interrupt.

The HP core boots, prints its wake count, and enters deep sleep via
``sys_poweroff()``. A ``RTC_DATA_ATTR`` variable tracks the number of
wakeups across deep sleep cycles.

By default, the sample uses GPIO0 (LP_IO_0) with a low-level trigger
and internal pull-up enabled. Any LP GPIO pin (GPIO0-GPIO7) can be used
by changing the ``wakeup-pin`` alias in the board overlay.

How it works
************

1. HP core loads LP core binary, configures LP IO as the wakeup source,
   and enters deep sleep.
2. LP core boots, signals the HP core via
   ``ulp_lp_core_wakeup_main_processor()``, configures the GPIO pin
   as input with pull-up and low-level interrupt, then halts.
3. When the pin is pulled low, the LP core resets and repeats from
   step 2, waking the HP core again.

Wiring
******

Connect a button or jumper wire to GPIO0. The internal pull-up is
enabled, so the pin is high by default. Briefly touch the pin to GND
to trigger a wakeup.

Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/gpio_wakeup
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/gpio_wakeup
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

    First boot, LP core will wake us on LP GPIO edge
    HP core entering deep sleep...

After touching GPIO0 to GND:

.. code-block:: console

    Woke up from LP core (LP IO)! Wake count: 1
    HP core entering deep sleep...
