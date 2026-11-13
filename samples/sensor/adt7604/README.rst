.. zephyr:code-sample:: adt7604
   :name: ADT7604 temperature and resistance sensor
   :relevant-api: sensor_interface

   Read temperature and resistance data from an ADT7604 sensor using the sensor API.

Overview
********

This sample application demonstrates how to use the ADT7604 20-channel temperature
and resistance sensor using the sensor read/decode API..

The sample performs two separate reads per loop iteration:

1. **Temperature channels** (SENSOR_CHAN_AMBIENT_TEMP) - RTD and thermistor sensor types
2. **Resistance channels** (SENSOR_CHAN_RESISTANCE) - copper-trace sensor types

The decoder returns one reading per channel instance. The print functions iterate
through channel indices until no more instances are found, automatically discovering
all configured channels.

The sample also demonstrates how to configure custom sensor data tables for copper
trace sensors using the ``adt7604_write_custom_sensor_data_table()`` API.

Building and Running
********************

This sample requires an ADT7604 sensor connected via SPI. It has been tested on
the MAX32655EVKIT board with proper devicetree configuration.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/adt7604
   :board: max32655evkit/max32655/m4
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS ***
   [00:00:00.010,000] <inf> adt7604: ADT7604 sample started
   Temperature channels:
     temp[0]:  25.125000 °C
     temp[1]:  24.875000 °C
   Resistance channels:
    resistance[0]: 0.621116 Ohm
    resistance[1]: 0.631796 Ohm
    resistance[2]: 49.150999 Ohm
    resistance[3]: 2097151.000000 Ohm
    resistance[4]: 2097151.000000 Ohm
   Temperature channels:
     temp[0]:  25.250000 °C
     temp[1]:  24.937500 °C
   Resistance channels:
    resistance[0]: 0.621116 Ohm
    resistance[1]: 0.631796 Ohm
    resistance[2]: 49.150999 Ohm
    resistance[3]: 2097151.000000 Ohm
    resistance[4]: 2097151.000000 Ohm

   <repeats every 5 seconds>

References
**********

- ADT7604: https://www.analog.com/adt7604
