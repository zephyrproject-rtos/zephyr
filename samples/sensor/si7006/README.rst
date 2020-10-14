.. _si7006:

Si7006 Humidity and Pressure Sensor
###################################

Overview
********

This sample periodically reads temperature and humidity from a
Si7006-family sensor and displays it on the console.

See the `Si7006 datasheet`_, `Si7013 datasheet`_, `Si7020 datasheet`_
or `Si7021 datasheet`_ for more details about the sensors.

Building and Running
********************

This sample application uses an Si7006/Si7013/Si7020/Si7021 sensor
connected to a board via I2C.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/si7006
   :board: efm32gg11_wlstk6121a
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses ``screen`` on the serial port ``/dev/ttyACM0``:

.. code-block:: console

        $ sudo screen /dev/ttyACM0

.. code-block:: console

	[00:00:00.018,000] <dbg> si7006.si7006_channel_get: temperature = val1:32, val2:640000
	[00:00:00.018,000] <dbg> si7006.si7006_channel_get: humidity = val1:18, val2:0
	temp: 32.640000; humidity: 18.000000
	[00:00:01.036,000] <dbg> si7006.si7006_channel_get: temperature = val1:32, val2:640000
	[00:00:01.036,000] <dbg> si7006.si7006_channel_get: humidity = val1:18, val2:0
	temp: 32.640000; humidity: 18.000000

.. _Si7006 datasheet: https://www.silabs.com/documents/public/data-sheets/Si7006-A20.pdf
.. _Si7013 datasheet: https://www.silabs.com/documents/public/data-sheets/Si7013-A20.pdf
.. _Si7020 datasheet: https://www.silabs.com/documents/public/data-sheets/Si7020-A20.pdf
.. _Si7021 datasheet: https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf
