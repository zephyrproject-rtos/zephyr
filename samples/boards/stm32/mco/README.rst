.. _samples_boards_stm32_mco:

STM32 MCO example
#################

Overview
********

This sample is a minimum application to demonstrate how to output one of the internal clocks for
external use by the application.

Requirements
************

The SoC should support MCO functionality and use a pin that has the MCO alternate function.
To support another board, add an overlay in boards folder.
Make sure that the output clock is enabled in dts overlay file.


Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/mco
   :board: nucleo_u5a5zj_q
   :goals: build flash

After flashing, the LSE clock will be output on the MCO pin enabled in Device Tree.
The clock can be observed using a probing device, such as a logic analyzer.
