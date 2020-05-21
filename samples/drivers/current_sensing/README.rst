.. _current_sensing:

Current Sensing
###############

Overview
********
This is a sample app to interface with TI INA219 power monitor.
The values used in the app are for use on Adafruit's breakout board
(https://www.adafruit.com/products/904).

This assumes the slave address is 0x40, where A0 and A1 are all tied to ground.

Building and Running
********************

This project outputs measured values to the console. It can be built and
executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/current_sensing
   :host-os: unix
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

	Bus Voltage: 3300 mV
	Shunt Voltage: 100 uV
	Current: 5000 uA
	Power: 16500 uW
