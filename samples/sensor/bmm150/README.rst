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

This sample application uses an BMM150 sensor connected to a board via I2C.
Sensor has multiple pins so you need to connect according to connection diagram given in
`bmm150 datasheet`_ at page 41.

.. code-block:: console

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmm150
   :board: nrf52840_pca10056
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
Here I am using picocom program to open output. Check which tty device it is.
In my case it is ttyACM0

.. code-block:: console

        $ sudo picocom -D /dev/ttyACM0

.. code-block:: console

        ( x y z ) = ( -0.390625  0.087500  -0.390625 )
        ( x y z ) = ( -0.275000  0.115625  -0.275000 )
        ( x y z ) = ( -0.281250  0.125000  -0.281250 )
        ( x y z ) = ( -0.287500  0.134375  -0.287500 )

.. _bmm150 datasheet: http://www.mouser.com/ds/2/783/BST-BMM150-DS001-01-786480.pdf
