.. zephyr:code-sample:: max30011
   :name: MAX30011 Biopotential and Bioimpedance AFE
   :relevant-api: sensor_interface

   TODO: This is not the README of max30011
   Get temperature, current, and voltage from the MAX30011 sensor using SPI.

Overview
********

This is a sample application that only reads periodically reads the temperature,
pstat current, chronoamperometry current, and voltage data from the MAX30011
sensor, displaying the results on the console. This sample does not include
the correct hardware setup for the electrochemical sensor, but it does
demonstrate how to read the data from the sensor.

Chronoamperometry Measurement
==============================

Chronoamperometry mode consists of a series of current measurements. A hack is
done to use **sensor_channel_get()** to read the measurement data with specific
indices.

Set **val2** of **struct sensor_value** to the index. The **sensor_channel_get()**
function will return the measurement data for the specified index.

System Voltage measurements
===========================

System voltage measurements consists of multiple voltage sources. A hack is
done to use **sensor_channel_get()** to read the measurement data with a specific
voltage source.

Set **val2** of **struct sensor_value** to the voltage source. The
**sensor_channel_get()** function will return the measurement data for the
specified voltage source. The voltage sources are defined in
**include/zephyr/drivers/sensor/max30011.h** as follows:

- **WE1_VOLTAGE_SELECT**
- **RE1_VOLTAGE_SELECT**
- **CE1_VOLTAGE_SELECT**
- **GR1_VOLTAGE_SELECT**
- **GPIO1_VOLTAGE_SELECT**
- **GPIO2_VOLTAGE_SELECT**
- **VBAT_VOLTAGE_SELECT**
- **VREF_VOLTAGE_SELECT**
- **VDD_VOLTAGE_SELECT**

Wiring
******

Connect **SPI** pins to the MAX30011 sensor. Additionally, provide **3.3V** and
**GND** to the sensor.

References
**********

 - MAX30011: https://www.analog.com/max30011

Building and Running
********************

This sample can be built with any board that supports SPI. Sample overlays are
provided for the NUCLEO-U575ZI-Q board, MAX32665 FTHR board, and MAX32655 EVKIT
board.

Building on NUCLEO-U575ZI-Q board
=================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max30011
   :board: nucleo_u575zi_q
   :goals: build
   :compact:

Building on MAX32655 EVKIT board
================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max30011
   :board: max32655evkit_max32655_m4
   :goals: build
   :compact:

Building on MAX32655 FTHR board
===============================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max30011
   :board: max32655fthr_max32655_m4
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Fetching all channels...
   PSTAT 0: 1.503906 nA
   PSTAT 1: 1.503906 nA
   Temperature: 31.000000 Cel
   WE1 Voltage: 1.004140 V
   CE1 Voltage: 0.027226 V
   GR1 Voltage: 1.003945 V
   GPIO1 Voltage: 0.000781 V
   VDD Voltage: 1.792968 V
   Chrono A [0]: 0.265625 nA
   Chrono A [1]: 0.252929 nA
   Chrono A [2]: 0.231445 nA
   Chrono A [3]: 0.256835 nA
   Chrono A [4]: 0.258789 nA
   Chrono A [5]: 0.277343 nA
   Chrono A [6]: 0.253906 nA
   Chrono A [7]: 0.236328 nA
   Chrono A [8]: 0.236328 nA
   Chrono A [9]: 0.241210 nA
   Chrono A [10]: 0.245117 nA
   Chrono A [11]: 0.218750 nA
   Chrono A [12]: 0.244140 nA
   Chrono A [13]: 0.275390 nA
   Chrono A [14]: 0.263671 nA
   Chrono B [0]: 0.263671 nA
   Chrono B [1]: 0.255859 nA
   Chrono B [2]: 0.265625 nA
   Chrono B [3]: 0.261718 nA
   Chrono B [4]: 0.252929 nA
   Chrono B [5]: 0.267578 nA
   Chrono B [6]: 0.233398 nA
   Chrono B [7]: 0.291015 nA
   Chrono B [8]: 0.276367 nA
   Chrono B [9]: 0.256835 nA
   Chrono B [10]: 0.265625 nA
   Chrono B [11]: 0.226562 nA
   Chrono B [12]: 0.266601 nA
   Chrono B [13]: 0.275390 nA
   Chrono B [14]: 0.227539 nA

   <repeats endlessly every second>
