.. _bmm150:

BMM150 Geomagnetic Sensor
#########################

Overview
********

This sample application periodically reads magnetometer (X, Y, Z) data from
the first available device that implements SENSOR_CHAN_MAGN_* (predefined array
of device names). This sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

This sample application uses an BMM150 sensor connected to an Arduino 101 board via I2C.
Sensor has multiple pins so you need to connect according to connection diagram given in
`bmm150 datasheet`_ at page 41.
There are two processor cores (x86 and ARC) on the Arduino 101. You will need to
flash both this sample's code on the ARC core (using the ``arduino101_ss`` board target),
and stub code on the x86 core (using the ``arduino_101`` board target), as shown below.

.. code-block:: console

.. zephyr-app-commands::
   :zephyr-app: samples/sensors/bmm150
   :board: arduino_101_sss
   :goals: flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: tests/booting/stub
   :board: arduino_101
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
Here I am using picocom program to open output. Check which tty device it is.
In my case it is ttyUSB0

.. code-block:: console

        $ sudo picocom -D /dev/ttyUSB0

.. code-block:: console

        ( x y z ) = ( -0.390625  0.087500  -0.390625 )
        ( x y z ) = ( -0.275000  0.115625  -0.275000 )
        ( x y z ) = ( -0.281250  0.125000  -0.281250 )
        ( x y z ) = ( -0.287500  0.134375  -0.287500 )

.. _bmm150 datasheet: http://www.mouser.com/ds/2/783/BST-BMM150-DS001-01-786480.pdf
