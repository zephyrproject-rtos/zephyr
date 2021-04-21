.. _ccs811:

CCS811 Indoor Air Quality Sensor
################################

Overview
********

The CCS811 digital gas sensor detects volatile organic compounds (VOCs)
for indoor air quality measurements. VOCs are often categorized as
pollutants and/or sensory irritants and can come from a variety of
sources such as construction materials (paint and carpet), machines
(copiers and processors), and even people (breathing and smoking).  It
estimates carbon dioxide (CO2) levels where the main source of VOCs is
human presence.

Building and Running
********************

Building and Running on thingy52_nrf52832
=========================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ccs811
   :board: thingy52_nrf52832
   :goals: build flash
   :compact:


Sample Output
=============

The sample output below is from a `Nordic Thingy:52
<https://www.nordicsemi.com/Software-and-tools/Prototyping-platforms/Nordic-Thingy-52>`_
(thingy52_nrf52832) that includes this sensor (and others).
After a soft reset, there is a 5-second startup period
where readings are unstable, and then we can see steady
reported measurements of about 400 ppm eC02 and 0 ppb eTVOC.

.. code-block::console

   *** Booting Zephyr OS build zephyr-v2.1.0-310-g32a3e9907bab  ***
   device is 0x20001088, name is CCS811
   HW 12; FW Boot 1000 App 1100 ; mode 10

   [0:00:00.046]: CCS811: 65021 ppm eCO2; 65021 ppb eTVOC
   Voltage: 0.000000V; Current: 0.000000A
   BASELINE fff4
   Timed fetch got 0

   [0:00:01.059]: CCS811: 65021 ppm eCO2; 65021 ppb eTVOC
   Voltage: 0.000000V; Current: 0.000000A
   BASELINE fff4
   Timed fetch got 0
   Timed fetch got stale data
   Timed fetch got stale data
   Timed fetch got stale data

   [0:00:05.084]: CCS811: 400 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.677040V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0

   [0:00:06.096]: CCS811: 405 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.675428V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0

   [0:00:07.108]: CCS811: 400 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.677040V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0

   [0:00:08.121]: CCS811: 400 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.677040V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0

   [0:00:09.133]: CCS811: 400 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.677040V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0

   [0:00:10.145]: CCS811: 400 ppm eCO2; 0 ppb eTVOC
   Voltage: 0.677040V; Current: 0.000014A
   BASELINE 8384
   Timed fetch got 0
