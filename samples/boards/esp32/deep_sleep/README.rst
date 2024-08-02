.. _esp32-deep-sleep-sample:

ESP32 Deep Sleep demo
#####################

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

In this demo, Timer is the only wake-up source that cannot be disabled via a
Kconfig option. The target SoC will always repeat the following: enable Timer
as wake-up source, deep sleep for 20 seconds, wake up.

Requirements
************

This example should be able to run on any commonly available
:ref:`esp32_devkitc_wroom` development board without any extra hardware if
only ``Timer`` is used as wakeup source.

However, when ``EXT1`` is also enabled, GPIO2 and GPIO4 should be pulled-down
by external resistors to avoid floating pins. When triggering a wake up, one
or both of the pins must be set to high. Note that floating pins may trigger
a wake up.

The same connection logic used on ``EXT1`` should be applied when ``GPIO`` is
enabled as wake-up source.

To enable or disable ``EXT1``, edit ``CONFIG_EXAMPLE_EXT1_WAKEUP`` on demo's
``prj.conf`` file. By default, this wake up source is enabled. Follow similar
steps to enable or disable ``GPIO`` by editing ``CONFIG_EXAMPLE_GPIO_WAKEUP``.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/esp32/deep_sleep
   :board: esp32_devkitc_wroom/esp32/procpu
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

   *** Booting Zephyr OS build zephyr-v3.1.0-3667-gb42e2b225ecf  ***

   Wake up from GPIO 2
   Enabling timer wakeup, 20s
   Enabling EXT1 wakeup on pins GPIO2, GPIO4
   Powering off
