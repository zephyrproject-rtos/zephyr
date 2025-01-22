.. zephyr:code-sample:: sensor_clock
   :name: Sensor Clock
   :relevant-api: sensor_interface

   Test and debug Sensor Clock functionality.

Overview
********

This sample application demonstrates how to select the sensor clock source
and utilize the Sensor Clock API.

Building and Running
********************

The sample below uses the :ref:`nrf52840dk_nrf52840` and :ref:`nrf52833dk_nrf52833` boards.

To run this sample, ensure the following configurations:

    * Enable one of the Kconfig options:
      :kconfig:option:`CONFIG_SENSOR_CLOCK_COUNTER`,
      :kconfig:option:`CONFIG_SENSOR_CLOCK_RTC`, or
      :kconfig:option:`CONFIG_SENSOR_CLOCK_SYSTEM`.

Build and run the sample with the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/clock
   :board: <board to use>
   :goals: build flash

Sample Output
=============

The application will print the current sensor clock cycles and
their corresponding time in nanoseconds.

.. code-block:: console

   Cycles: 143783087
   Nanoseconds: 8986442937
   Cycles: 159776386
   Nanoseconds: 9986024125
   Cycles: 175772543
   Nanoseconds: 10985783937
   Cycles: 191771203
   Nanoseconds: 11985700187
   Cycles: 207758870
   Nanoseconds: 12984929375
   Cycles: 223752074
   Nanoseconds: 13984504625

   ...
