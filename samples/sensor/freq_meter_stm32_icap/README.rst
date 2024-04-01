.. _freq_meter_stm32_icap:

Frequency Meter using STM32 Input Capture
#########################################

Overview
********

A sensor application that demonstrates how to use the STM32 Input Capture timer
to create a Frequency Meter.

Building and Running
********************

This project outputs sensor data to the console. It requires an stm32 board.
This was tested using :ref:`nucleo_g474re_board` and :ref:`nucleo_f767zi_board`
boards.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/freq_meter_stm32_icap
   :board: nucleo_g474re/stm32g474xx
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Freq=100Hz      RPM=6000
   Freq=100Hz      RPM=6000
   Freq=100Hz      RPM=6000

<repeats endlessly>
