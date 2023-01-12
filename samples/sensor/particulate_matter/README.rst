.. _particulate_matter:

Laser Particle Sensor
#####################

Overview
********

This sample shows how to use the Zephyr Sensors API driver for a
Particulate Matter (PM) sensor, like the SN-GCJA5.

The sample detects values for PM1, PM2.5, PM10 and the particle counts
periodically and prints them once a second to the console.

Building and Running
********************

SN-GCJA5 via Arduino I2C pins
=============================

Please see detailed sensor information for `SN-GCJA5 laser particle sensor`_.

If you wire the sensor to I2C on an Arduino header, build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/particulate_matter
   :goals: build flash
   :board: pan1781_evb
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay
:zephyr_file:`samples/sensor/particulate_matter/arduino_i2c.overlay`
works on any board with a properly configured Arduino pin-compatible I2C
peripheral. To build for another board, change "pan1781_evb" above to that
board's name.

Sample Output
=============

The sensor, samples and prints out the values to the serial console
once a second. Following output shows some example values.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-3320-g30fd0a5a3b7f ***
   Found device "sngcja5", getting sensor data
   new sample:
   pc0_5: 5
   pc1_0: 29
   pc2_5: 3
   pc5_0: 0
   pc7_5: 0
   pc10_0: 0
   pm1_0: 3.127000
   pm2_5: 3.310000
   pm10_0: 3.723000

.. target-notes::

.. _`SN-GCJA5 laser particle sensor`: https://industry.panasonic.eu/products/components/sensors/particulate-matter-sensor
