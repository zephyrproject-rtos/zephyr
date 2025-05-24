.. zephyr:code-sample:: tofsense
   :name: TOFSense Time Of Flight sensor
   :relevant-api: sensor_interface

   Get distance data from a TOFSense sensor (interrupt mode).

Overview
********

This sample periodically measures distance between TOFSense sensor
and target. The result is displayed on the console.

Requirements
************

This sample uses the TOFSense sensor with its factory default settings, on the UART interface.

References
**********

 - Datasheet: https://ftp.nooploop.com/downloads/tofsense/TOFSense_Datasheet_V3.0_en.pdf
 - User manual: https://ftp.nooploop.com/downloads/tofsense/TOFSense_User_Manual_V3.0_en.pdf

Building and Running
********************

This project outputs sensor data to the console. It requires a TOFSense
sensor to be plugged on UART2 (UART on CN9 connector of all ST nucleo boards).

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tofsense/
   :board: nucleo_u575zi_q
   :goals: build flash


Sample Output
=============

.. code-block:: console

   distance is 000 mm

   distance is 1888 mm

   <repeats endlessly every 5 seconds>
