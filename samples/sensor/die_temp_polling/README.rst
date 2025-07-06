.. zephyr:code-sample:: die_temp_polling
   :name: CPU die temperature polling
   :relevant-api: sensor_interface

   Get CPU die temperature data from a sensor using polling.

Overview
********

This sample periodically reads temperature from the CPU Die
temperature sensor and display the results.

Building and Running
********************

To run this sample, enable the sensor node that supports ``SENSOR_CHAN_DIE_TEMP``
and create an alias named ``die-temp0`` to link to the node.
The tail ``0`` is the sensor number.  This sample support up to 15 sensors.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/die_temp_polling
   :board: rpi_pico
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    CPU Die temperature[dietemp]: 22.6 °C
    CPU Die temperature[dietemp]: 22.8 °C
    CPU Die temperature[dietemp]: 23.1 °C
