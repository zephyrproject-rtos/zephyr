.. zephyr:code-sample:: pzem004t
   :name: PZEM004T Multi function AC Sensor sample
   :relevant-api: sensor_interface modbus

   Get AC sample data from a PZEM004T sensor (Voltage, Current, Power,
   Energy, Frequency, Power Factor) using modbus protocol over UART.
   Get sensor parameter like power alarm threshold and modbus address and also
   set sensor parameter like power alarm threshold and modbus address.

Overview
********

A sample showcasing various features of the PZEM004T sensor.

* Read Measurement Values
      Read the measurement values from the sensor. The values include voltage,
      current, power, energy, frequency, and power factor.
* Read PZEM004T Parameters
      Read the parameters of the PZEM004T sensor. The parameters include
      power alarm threshold and modbus address.
* Set PZEM004T Parameters
      Set the parameters of the PZEM004T sensor. The parameters include
      power alarm threshold and modbus address.
* Reset Energy
      Reset the energy counter of the PZEM004T sensor.

Requirements
************

PZEM004T wired to your board UART bus, for example:

- :zephyr:board:`nucleo_g071rb`

Building and Running
********************
Example building for :zephyr:board:`nucleo_g071rb`:

This sample can be built with any board that supports UART. A sample overlay is
provided for the NUCLEO-G071RB board.

The sample can be built for various configurations like reading measurement values,
reading parameters, setting parameters, and resetting energy.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pzem004t
   :board: nucleo_g071rb
   :gen-args: -DCONFIG_READ_MEASUREMENT_VALUES=y
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pzem004t
   :board: nucleo_g071rb
   :gen-args: -DCONFIG_READ_SENSOR_PARAMETERS=y
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pzem004t
   :board: nucleo_g071rb
   :gen-args: -DCONFIG_RESET_ENERGY=y
   :goals: build flash


Sample Output
=============

if DCONFIG_READ_MEASUREMENT_VALUES=y is set, the output will be similar to:
.. code-block:: console

   Voltage: 235.9 V
   Current: 0.423 A
   Power: 93.2 W
   Frequency: 50.0 Hz
   Energy: 34.0 Wh
   Power Factor: 0.93
   Alarm Status: 0

   <repeats endlessly every second>

if DCONFIG_READ_SENSOR_PARAMETERS=y is set, the output will be similar to:
.. code-block:: console

   Power Alarm Threshold: 1000 W
   Modbus RTU Address: 0x01

if DCONFIG_SET_SENSOR_PARAMETERS=y is set, the output will be similar to:
.. code-block:: console

   Power alarm threshold set to: 1000 W
   Modbus RTU address set to: 0x01

if DCONFIG_RESET_ENERGY=y is set, the output will be similar to:
.. code-block:: console

   Energy reset successfully
