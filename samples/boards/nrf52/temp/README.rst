.. _temp_nrf5:

nRF5 Die Temperature Monitor
########################################

Overview
********
The Temperature Example shows how to use the temperature sensor which is
present in nRF5 SoC dies to print temperature data.


Requirements
************

Only available on boards with nRF5 chips.


Building and Running
********************

 This project outputs sensor data to the console. It acquires die
 temperature data and prints the measurements.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/temp
   :board: nrf52840_pca10056
   :goals: build
   :compact:

Sample Output
=============

 .. code-block:: console

   Temperature:26.500000 C
   Temperature:26.500000 C
   Temperature:26.500000 C
   Temperature:26.500000 C
   Temperature:26.500000 C
   Temperature:26.500000 C
    <repeats endlessly every second>
