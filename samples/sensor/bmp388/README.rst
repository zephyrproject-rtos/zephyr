.. _bmp388:

BMP388 Pressure Sensor
######################

Overview
********

This sample application periodically reads temperature and pressure data from
the first available device that implements SENSOR_CHAN_AMBIENT_TEMP and
SENSOR_CHAN_PRESS. This sample checks the sensor in polling mode (without
interrupt trigger). Finally it prints out the temperature and pressure reading,
along with the relative altitude in meters calculated with the barometric
formula.

Building and Running
********************

This sample application uses an BMP388 sensor connected to a board via I2C.
Connect the sensor pins according to the I2C connection paragraph given in the
`bmp388 datasheet`_ at page 39.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmp388
   :board: nucleo_l073rz
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses ``minicom`` on the serial port ``/dev/ttyACM0``:

.. code-block:: console

        $ sudo minicom -D /dev/ttyUSB0

.. code-block:: console

        dev 0x200012d0 name BMP388
        Zeroing the pressure
        press: 101.853300 kPa; temp: 22.642018 oC; altitude:  0.0 m
        press: 101.853200 kPa; temp: 22.646600 oC; altitude:  0.1 m
        press: 101.853400 kPa; temp: 22.650336 oC; altitude:  0.0 m

.. _bmp388 datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
