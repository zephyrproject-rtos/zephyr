.. _isl29035:

ISL29035: Digital Light Sensor
##############################

Overview
********

If trigger is not enabled the sample displays measured light intensity
every 2 seconds.

If trigger is enabled the sample displays light intensity from the
ISL29035 sensor every 10 seconds if it is within +/- 50 lux of the last
read sample.  If the sensor detects an intensity outside that range the
application wakes, displays the intensity, resets the intensity range
window to center on the new value, then continues as before.

Requirements
************

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to connect the sensor to the I2C bus and
identify the interrupt signal.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/isl29035
   :board: nrf52dk_nrf52832
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.1.0-335-gfe020d937d43  ***
   ALERT 365 lux outside range centered on 0 lux.
   Next alert outside 315 .. 415
   [0:00:00.018] Ambient light sense: 365.234
   [0:00:10.023] Ambient light sense: 361.084
   ALERT 302 lux outside range centered on 365 lux.
   Next alert outside 252 .. 352
   [0:00:13.276] Ambient light sense: 302.734
   ALERT 247 lux outside range centered on 302 lux.
   Next alert outside 197 .. 297
   [0:00:14.619] Ambient light sense: 247.62
   ALERT 187 lux outside range centered on 247 lux.
   Next alert outside 137 .. 237
   [0:00:16.141] Ambient light sense: 187.927
   ALERT 126 lux outside range centered on 187 lux.
   Next alert outside 76 .. 176
   [0:00:16.410] Ambient light sense: 126.953
   ALERT 181 lux outside range centered on 126 lux.
   Next alert outside 131 .. 231
   [0:00:17.843] Ambient light sense: 181.03
   ALERT 235 lux outside range centered on 181 lux.
   Next alert outside 185 .. 285
   [0:00:18.022] Ambient light sense: 235.779
   ALERT 301 lux outside range centered on 235 lux.
   Next alert outside 251 .. 351
   [0:00:23.126] Ambient light sense: 301.758
   ALERT 353 lux outside range centered on 301 lux.
   Next alert outside 303 .. 403
   [0:00:23.305] Ambient light sense: 353.333
   [0:00:33.310] Ambient light sense: 365.112

   <repeats as necessary>
