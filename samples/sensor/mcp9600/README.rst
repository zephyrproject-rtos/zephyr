.. _mcp9600:

mcp9600 Temperature Sensor
###################################

Overview
********

This sample application periodically reads temperature data from the first
available device that implements SENSOR_CHAN_AMBIENT_TEMP.

Building and Running
********************

This sample application uses an mcp9600 sensor connected to a board via I2C.
We used the Sparkfun `MCP9600 Sensor`_ breakout board. The datasheet is
available at `MCP9600 Datasheet`_

.. _`MCP9600 Sensor`: https://www.sparkfun.com/products/16295

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcp9600
   :board: frdm_kw41z
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses screen on OS X with a frdm_kw41z development board. On
other OS's this is usually ACM0.

.. code-block:: console

        $ screen /dev/tty.usbmodem0006210000001 115200

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v2.4.0-2710-g5358a11687a3  ***
        Found device "MCP9600"
        Temperature: 20.3125 C
        Temperature: 20.5000 C
        Temperature: 20.7500 C
        Temperature: 20.8125 C
        Temperature: 20.6875 C
        Temperature: 20.8125 C
        Temperature: 20.8750 C
        Temperature: 20.6250 C

.. _`MCP9600 Datasheet`: https://www.microchip.com/wwwproducts/en/MCP9600
