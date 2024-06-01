.. _pms7003:

PMS Particle Concentration Sensor
###################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Plantower pms7003`_ Particle Concentration Sensor.

.. _Plantower pms7003:
   https://www.plantower.com/en/products_33/76.html

The same driver can also be used for different pms sensors of series pmsX003. Forexample, pms5003 and pms6003

The sample code periodically reads various particulate matter concentrations and particle counts from the
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
   :zephyr-app: samples/sensor/pms7003
   :board: esp32_devkitc_wroom
   :goals: build
   :compact:


Sample Output
=============

 .. code-block:: console
*** Booting Zephyr OS build v3.6.0-5099-g127cb9edb6c6 ***
[00:00:00.919,000] <inf> MAIN: pm1.0_cf = 38.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm2.5_cf = 52.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm10_cf = 56.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm1.0_atm = 29.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm2.5_atm = 42.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm10_atm = 52.00 µg/m³
[00:00:00.919,000] <inf> MAIN: pm0.3_count = 6399 particles/0.1L
[00:00:00.919,000] <inf> MAIN: pm0.5_count = 1806 particles/0.1L
[00:00:00.919,000] <inf> MAIN: pm1.0_count = 300 particles/0.1L
[00:00:00.919,000] <inf> MAIN: pm2.5_count = 20 particles/0.1L
[00:00:00.919,000] <inf> MAIN: pm5.0_count = 4 particles/0.1L
[00:00:00.919,000] <inf> MAIN: pm10_count = 4 particles/0.1L

<repeats endlessly every 10 seconds>
