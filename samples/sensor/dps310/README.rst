.. _dps310:

DPS310 Temperature and Pressure Sensor
######################################

Overview
********

This sample application periodically reads temperature and pressure data from
the first available device that implements SENSOR_CHAN_AMBIENT_TEMP and
SENSOR_CHAN_PRESS. This sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

This sample application uses an DPS310 sensor connected to a board via I2C.
Connect the sensor pins according to the connection diagram given in the
`dps310 datasheet`_ at page 18 figure 7.

Build and flash this sample (for example, for the nrf52840dk/nrf52840 board)
using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/dps310
   :board: nrf52840dk/nrf52840
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample, any serial console program can be used.
This example uses ``picocom`` on the serial port ``/dev/ttyUSB0``:

.. code-block:: console

        $ sudo picocom -D /dev/ttyUSB0

.. code-block:: console

        temp: 23.774363; press: 97.354728
        temp: 23.777492; press: 97.353904
        temp: 23.784646; press: 97.354064

.. _dps310 datasheet: https://www.infineon.com/dgdl/Infineon-DPS310-DataSheet-v01_01-EN.pdf?fileId=5546d462576f34750157750826c42242
