.. _max17262:

MAX17262 Fuel Gauge Sensor
###################################

Overview
********

This sample application periodically reads voltage, current and temperature
data from the MAX17262 device that implements SENSOR_CHAN_GAUGE_VOLTAGE,
SENSOR_CHAN_GAUGE_AVG_CURRENT, and SENSOR_CHAN_GAUGE_TEMP.

Requirements
************

The MAX17262 is an ultra-low power fuel-gauge IC which implements the Maxim
ModelGauge m5 algorithm. The IC monitors a single-cell battery pack and
supports internal current sensing for up to 3.1A pulse current. The IC
provides best performance for batteries with 100mAhr to 6Ahr capacity.

This sample requires a board which provides a configuration for Arduino
connectors and defines node aliases for the I2C interface.
For more info about the node structure see
:zephyr_file:`samples/sensor/max17262/app.overlay`

Building and Running
********************

This sample application uses an MAX17262 sensor connected to a board via I2C.
Connect the sensor pins according to the connection diagram given in the
`max17262 datasheet`_.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max17262
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Sample Output
=============
To check output of this sample , any serial console program can be used.
This example uses ``picocom`` on the serial port ``/dev/ttyUSB0``:

.. code-block:: console

        $ sudo picocom -D /dev/ttyUSB0

.. code-block:: console

        V: 3.626406 V; I: -3.437500 mA; T: 28.011718 °C
        V: 3.626406 V; I: -3.437500 mA; T: 28.011718 °C
        V: 3.626406 V; I: -3.437500 mA; T: 28.011718 °C

References
***********

.. _max17262 datasheet: https://datasheets.maximintegrated.com/en/ds/MAX17262.pdf
