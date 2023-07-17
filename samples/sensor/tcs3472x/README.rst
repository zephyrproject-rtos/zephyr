.. _tcs3472x:

TCS3472x Color Sensor
#####################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`AMS TCS3472x`_ color sensor.

.. _AMS TCS34725:
   https://ams.com/tcs34725

The sample periodically reads RGB data from the
first available TCS3472x device discovered in the system. The sample checks the
sensor in polling mode (without interrupt trigger).

Building and Running
********************

The sample can be configured to support TCS3472x sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "ams,tcs3472x";``. See
:dtcompatible:`ams,tcs3472x` for the devicetree binding and see below for
examples and common configurations.

Start by wiring the sensor pins according to the connection
diagram given in the `TCS34725 datasheet`_ at page 5.

.. _TCS34725 datasheet:
   https://ams.com/documents/20143/36005/TCS3472_DS000390_3-00.pdf/6fe47e15-e32f-7fa7-03cb-22935da44b26

TCS3472x via Arduino I2C pins
=============================

If you wired the sensor to an I2C peripheral on an Arduino header, build and
flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tcs3472x
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay :zephyr_file:`samples/sensor/tcs3472x/arduino_i2c.overlay`
works on any board with a properly configured Arduino pin-compatible I2C
peripheral.

TCS3472x via GPIO bitbanging I2C pins
=====================================

If you wired the sensor to Pins D6(SDA) and D7(SCL) on an Arduino header, build and
flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tcs3472x
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c_bitbang.overlay

The devicetree overlay :zephyr_file:`samples/sensor/tcs3472x/arduino_i2c_bitbang.overlay`
works on any board with a properly configured Arduino pin-compatible GPIO mapping.

Board-specific overlays
=======================

If your board's devicetree does not have a TCS3472x node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`esp32` using the
:zephyr_file:`samples/sensor/tcs3472x/boards/esp32.overlay`
overlay provided with this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tcs3472x
   :goals: build flash
   :board: esp32

Sample Output
=============

The sample prints output to the serial console. TCS3472x device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one TCS3472x sensor is connected to the standard Arduino I2C pins:

.. code-block:: none

   [00:00:00.000,000] <inf> TCS3472X: TCS34723/TCS34727 detected
   *** Booting Zephyr OS build 45f34ddeb72c ***
   Found "tcs34725@29", getting RGB values
   red: 1.090000; green: 1.000000; blue: 0.910000
   red: 1.090000; green: 1.090000; blue: 0.910000
   red: 1.090000; green: 1.090000; blue: 0.910000
