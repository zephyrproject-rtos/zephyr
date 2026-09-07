.. zephyr:code-sample:: uart-zero-latency-interrupt
   :name: UART zero latency interrupt
   :relevant-api: uart_interface

   Demonstrates how to setup zero latency interrupt for UART.

Overview
********

This sample demonstrates how to use STM32 UART serial driver with zero latency interrupt.

By default, the UART peripheral that is normally assigned to the Zephyr shell
is used, hence the majority of boards should be able to run this sample.

Building and Running
********************

Build and flash the sample as follows, changing ``stm32f3_disco`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/uart/zero_latency_interrupt
   :board: stm32f3_disco
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    ZLI enabled
