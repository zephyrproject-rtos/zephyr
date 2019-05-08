.. _temp_nrf5:

TEMP_nRF5: Die temperature Monitor
########################################

Overview
********
The Temperature Example shows how to use the temperature sensor which is
inside die and print the measurement.


Requirements
************

Only available on the nRF5 chipsets.


Building and Running
********************

 This project outputs sensor data to the console. It requires the die
 temperature data and print the measurement.

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
    <repeats endlessly every 1 seconds>
