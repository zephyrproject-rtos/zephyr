.. zephyr:code-sample:: esp32-deep-sleep
   :name: Deep Sleep

   Use deep sleep with wake on timer, GPIO, and EXT1 sources on ESP32.

Overview
********

The deep sleep mode of the ESP32 Series is a power saving mode that causes the
CPU, majority of RAM, and digital peripherals that are clocked from APB_CLK to
be powered off.

This sample shows how to set a wake up source, trigger deep sleep and then
make use of that pre-configured wake up source to bring the system back again.

The following wake up sources are demonstrated in this example:

1. ``Timer``: An RTC timer that can be programmed to trigger a wake up after
   a preset time. This example will trigger a wake up every 20 seconds.
2. ``EXT1``: External wake up 1 is tied to multiple RTC GPIOs. This example
   uses GPIO2 and GPIO4 to trigger a wake up with any one of the two pins are
   HIGH.
3. ``GPIO``: Only supported by some Espressif SoCs, in the case of ESP32-C3
   GPIOS0~5 can be used as wake-up sources.

The target SoC will always repeat the following: enable Timer as wake-up source,
deep sleep for CONFIG_EXAMPLE_WAKEUP_TIME_SEC seconds, and wake up.

Requirements
************

This example should be able to run on any commonly available
:zephyr:board:`esp32_devkitc` development board without any extra hardware if
only ``Timer`` is used as wakeup source.

However, when EXT1 is enabled, GPIO2 and GPIO4 must be configured with a
pull-down to avoid floating pins. This is typically done via the device tree
(for example, using GPIO_PULL_DOWN). When triggering a wake-up, one or
both pins must be driven high. Floating pins may trigger spurious wake-ups.

The GPIO pins and their electrical characteristics used for ``EXT1`` or ``GPIO``
wake-up (such as pin number, pull configuration, and active level) are defined
in the sample’s device tree overlay and can be modified for testing as needed.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/deep_sleep
   :board: esp32_devkitc/esp32/procpu
   :goals: build flash
   :compact:

Sample Output
=================
ESP32 core output
-----------------

With both wake up sources enabled, the console output will be as below. The
reset reason message depends on the wake up source used. The console output
sample below is for GPIO2.

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-4908-g23226d1f3a4e ***

   Wake up from GPIO 2
   Enabling timer wakeup, 5s
   Powering off
