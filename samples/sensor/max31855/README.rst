.. _max31855:

Maxim MAX31855 Cold-Junction Compensated Thermocouple-to-Digital Converter
##########################################################################

Overview
********

This sample application demonstrates how to use the Maxim MAX31855
cold-junction compensated thermocouple-to-digital converter to measure
temperature from a thermocouple and the die temperature in °C.

Building and Running
********************

This sample application uses the sensor connected to the SPI stated in the
max31855.overlay file.
Flash the binary to a board of choice with a sensor connected.
For example build for a particle_xenon board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max31855
   :board: particle_xenon
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

	dev 0x20000f98, name MAX31855
	ambient temp: 25.000000 C; die temp: 24.750000 C
	ambient temp: 25.000000 C; die temp: 24.687500 C
	ambient temp: 25.000000 C; die temp: 24.750000 C
