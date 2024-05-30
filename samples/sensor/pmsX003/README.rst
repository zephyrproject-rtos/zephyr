.. _pmsx003:

PMS Particle Concentration Sensor
###################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Plantower PMSX003`_ Particle Concentration Sensor.

The "X" in PMSX003 stands for any sensor in the series, such as PMS5003, PMS6003, or PMS7003. 

The sample periodically reads various particulate matter concentrations and particle counts from the
PMS sensors attached to the uart pins. The sample checks the sensor in polling mode (without interrupt trigger).

The sensor provides the following measurements:

- **PM1.0_ATM:** 1.0 micrometer Particulate Matter (under atmospheric environment), in µg/m³
- **PM2.5_ATM:** 2.5 micrometer Particulate Matter (under atmospheric environment), in µg/m³
- **PM10.0_ATM:** 10.0 micrometer Particulate Matter (under atmospheric environment), in µg/m³
- **PM1.0_CF:** 1.0 micrometer Particulate Matter (CF=1, standard particle), in µg/m³
- **PM2.5_CF:** 2.5 micrometer Particulate Matter (CF=1, standard particle), in µg/m³
- **PM10.0_CF:** 10.0 micrometer Particulate Matter (CF=1, standard particle), in µg/m³
- **PM0.3_COUNT:** Number of particles with diameter beyond 0.3µm in 0.1L of air
- **PM0.5_COUNT:** Number of particles with diameter beyond 0.5µm in 0.1L of air
- **PM1.0_COUNT:** Number of particles with diameter beyond 1.0µm in 0.1L of air
- **PM2.5_COUNT:** Number of particles with diameter beyond 2.5µm in 0.1L of air
- **PM5.0_COUNT:** Number of particles with diameter beyond 5.0µm in 0.1L of air
- **PM10.0_COUNT:** Number of particles with diameter beyond 10.0µm in 0.1L of air

Building and Running
********************
 This project outputs sensor data to the console. It requires an esp32
 sensor.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/PMSX003
   :board: esp32_devkitc_wroom
   :goals: build
   :compact:


Sample Output
=============

 .. code-block:: console

*** Booting Zephyr OS build v3.6.0-4665-gea29ef36bbed ***
[00:00:01.013,000] <inf> MAIN: pm1.0_cf = 50.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm2.5_cf = 75.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm10_cf = 76.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm1.0_atm = 34.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm2.5_atm = 51.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm10_atm = 62.00 µg/m³
[00:00:01.013,000] <inf> MAIN: pm0.3_count = 8793 particles/0.1L
[00:00:01.013,000] <inf> MAIN: pm0.5_count = 2448 particles/0.1L
[00:00:01.013,000] <inf> MAIN: pm1.0_count = 456 particles/0.1L
[00:00:01.013,000] <inf> MAIN: pm2.5_count = 24 particles/0.1L
[00:00:01.013,000] <inf> MAIN: pm5.0_count = 2 particles/0.1L
[00:00:01.013,000] <inf> MAIN: pm10_count = 0 particles/0.1L

<repeats endlessly every 10 seconds>
