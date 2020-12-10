.. _bme280:

ACS71020
###################################

Overview
********

This sample application periodically reads RMS Voltage, current and power data from
the first available device that implements SENSOR_CHAN_VOLTAGE, SENSOR_CHAN_CURRENT,
and SENSOR_CHAN_POWER.

Building and Running
********************

This sample application uses an ACS71020 sensor connected to a board via I2C.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/acs71020
   :board: nrf52840dk_nrf52840
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used. This example uses screen on OS X.

.. code-block:: console

        $ screen /dev/tty.usbmodem114200 115200

.. code-block:: console





.. _bme280 datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf
