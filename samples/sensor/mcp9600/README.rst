.. _mcp9600:

mcp9600 Humidity and Pressure Sensor
###################################

Overview
********

This sample application periodically reads temperature, pressure and humidity data from
the first available device that implements SENSOR_CHAN_AMBIENT_TEMP, SENSOR_CHAN_PRESS,
and SENSOR_CHAN_HUMIDITY. This sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

This sample application uses an mcp9600 sensor connected to a board via I2C.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcp9600
   :board: frdm_kw41z
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses `screen on OS X with a frdm_kw41z development board.

.. code-block:: console

        $ screen /dev/tty.usbmodem0006210000001 115200

.. code-block:: console

	thermocouple temp: 24.125000; diff temp: -1.187500; cold temp: 24.937500
	thermocouple temp: 24.437500; diff temp: -1.500000; cold temp: 24.875000
	thermocouple temp: 33.062500; diff temp: 8.250000; cold temp: 24.937500
	thermocouple temp: 35.312500; diff temp: 10.500000; cold temp: 24.937500
