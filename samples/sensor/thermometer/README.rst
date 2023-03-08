.. _thermometer-samples:

Thermometer sample
##################

Overview
********

A simple thermometer example, which writes the temperature to the console once per second.

Building and Running
********************

It can be built and executed on bbc_microbit as follows:

.. zephyr-app-commands::
	:zephyr-app: samples/sensor/thermometer
	:board: bbc_microbit
	:goals: build
	:compact:

Sample Output
=============

.. code-block:: console

	Thermometer Example!
	arm temp device is 0x20000184, name is TEMP_0
	Temperature is 29.5C
	Temperature is 29.5C
	Temperature is 29.5C
