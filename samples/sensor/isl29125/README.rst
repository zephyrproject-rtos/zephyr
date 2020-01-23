.. _isl29125:

ISL29125: RGB Color sensor
##############################

Overview
********

If trigger is not enabled the sample displays measured light RGB
components every 2 seconds.

If trigger is enabled the sample shows reading which are out of
allowed window. It's configured to detect red component with values
lower then 1100 and higher then 2200 units.

Requirements
************

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to connect the sensor to the I2C bus and
identify the interrupt signal. It was tested with NRF52840 dev board
and MikroE Color 2 Shield. Provided overlay for this board assumes
that trigger line is connected to pin P1.01.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/isl29125
   :board: nrf52_pca10056
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   [00:00:00.001,800] <inf> ISL29125: Configuring trigger submodule
   *** Booting Zephyr OS build zephyr-v2.1.0-782-gf73069076be6  ***
   [00:00:00.004,516] <err> appLog: Main start
   [00:00:02.008,087] <inf> ISL29125: Threshold for channel RED
   [00:00:02.008,087] <inf> ISL29125: Lower threshold set to: 1100
   [00:00:02.011,138] <inf> appLog: Set lower red threshold to 1100, result: 0
   [00:00:02.011,169] <inf> ISL29125: Threshold for channel RED
   [00:00:02.011,169] <inf> ISL29125: Upper threshold set to: 2200
   [00:00:02.014,221] <inf> appLog: Set upper red threshold to 2200, result: 0
   [00:00:02.014,221] <inf> ISL29125: Trigger setup
   [00:00:02.014,251] <inf> appLog: Setup trigger, result: 0
   [00:00:15.678,619] <err> ISL29125: Trigger INT
   [00:00:15.680,908] <inf> appLog: Triggered, red value is 562 not in window (1100, 2200), st:0


   <repeats as necessary>
