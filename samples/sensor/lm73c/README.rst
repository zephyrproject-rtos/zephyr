.. _lm73c:

lm73c Temperature Sensor
###################################

Overview
********

This sample application periodically reads temperature data from the first
available device that implements SENSOR_CHAN_AMBIENT_TEMP. This
sample checks the sensor in polling mode (without interrupt trigger).

Building and Running
********************

This sample application uses an lm73c sensor connected to a board via I2C.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lm73c
   :board: frdm_kw41z
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses screen on OS X with a frdm_kw41z development board.

.. code-block:: console

        $ screen /dev/tty.usbmodem0006210000001 115200

.. code-block:: console

	<dbg> LM73C.lm73c_init: LM73C: Initializing
        LM73C: id (0x0190) = 190[00:00:00.002,000] <dbg> LM73C.lm73c_init: LM73C: Initialized
        *** Booting Zephyr OS build zephyr-v2.4.0-2710-g5358a11687a3  ***
        Found device "LM73C"
        Temperature: 23.25000 C
        Temperature: 23.21875 C
        Temperature: 23.15625 C
        Temperature: 23.12500 C
        Temperature: 23.03125 C
