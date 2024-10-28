.. zephyr:code-sample:: tmp108
   :name: TMP108 Temperature Sensor
   :relevant-api: sensor_interface

   Get temperature data from a TMP108 sensor (polling & trigger mode).

Description
***********

This sample writes the temperature to the console once every 3 seconds. There are
macro definitions included for turning off and on alerts if that is set up, and
also using low power one shot mode.

Requirements
************

A board with the :dtcompatible:`ti,tmp108` built in to its :ref:`devicetree <dt-guide>`,
or a devicetree overlay with such a node added.

Sample Output
=============

.. code-block:: console

	** Booting Zephyr OS build zephyr-v2.6.0-1923-g72bb75a360ce  ***
	TI TMP108 Example, arm
	Temperature is 22.875C
	Temperature is 22.875C
	Temperature is 22.875C
	Temperature is 22.875C
	Temperature is 22.875C
	Temperature is 22.875C
