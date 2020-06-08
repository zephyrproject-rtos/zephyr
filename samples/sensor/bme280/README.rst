.. _bme280:

BME280 Humidity and Pressure Sensor
###################################

Overview
********

This sample application periodically reads temperature, pressure and humidity data from
the first available device that implements SENSOR_CHAN_AMBIENT_TEMP, SENSOR_CHAN_PRESS,
and SENSOR_CHAN_HUMIDITY. This sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

This sample application uses an BME280 sensor connected to a board via I2C.
Connect the sensor pins according to the connection diagram given in the `bme280 datasheet`_
at page 38.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bme280
   :board: nrf52840dk_nrf52840
   :goals: flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses ``picocom`` on the serial port ``/dev/ttyACM0``:

.. code-block:: console

        $ sudo picocom -D /dev/ttyUSB0

.. code-block:: console

        temp: 27.950000; press: 100.571027; humidity: 61.014648
        temp: 27.940000; press: 100.570269; humidity: 61.012695
        temp: 27.950000; press: 100.570695; humidity: 61.002929

.. _bme280 datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf
