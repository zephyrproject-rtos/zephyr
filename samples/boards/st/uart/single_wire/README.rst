.. zephyr:code-sample:: uart-stm32-single-wire
   :name: Single-wire UART
   :relevant-api: uart_interface

   Use single-wire/half-duplex UART functionality of STM32 devices.

Overview
********

A simple application demonstrating how to use the single-wire/half-duplex
UART functionality of STM32 devices. The example runs on various STM32
boards with minimal adaptations.
Add a ``single_wire_uart_loopback`` fixture to your board in the hardware map to allow
twister to verify this sample's output automatically.

Hardware Setup
**************

You need to establish a physical connection between UART pins on the board.
Refer to the specific board overlay file for the exact pin connections.

Building and Running
********************

Build and flash as follows, replacing ``stm32f3_disco`` with your board:

 .. zephyr-app-commands::
    :zephyr-app: samples/boards/st/uart/single_wire
    :board: stm32f3_disco
    :goals: build flash
    :compact:

After flashing the console output should not show any failure reports,
but the following message repeated every 2s:

.. code-block:: none

    Received c
