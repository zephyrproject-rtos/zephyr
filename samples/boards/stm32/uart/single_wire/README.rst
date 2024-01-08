.. zephyr:code-sample:: uart-stm32-single-wire
   :name: STM32 single-wire UART
   :relevant-api: uart_interface

   Use single-wire/half-duplex UART functionality of STM32 devices.

Overview
********

A simple application demonstrating how to use the single wire / half-duplex UART
functionality of STM32. Without adaptions this example runs on STM32F3 discovery
board. You need to establish a physical connection between pins PA2 (USART2_TX) and
PC10 (UART4_TX).

Add a `single_wire_uart_loopback` fixture to your board in the hardware map to allow
twister to verify this sample's output automatically.

Building and Running
********************

Build and flash as follows, replacing ``stm32f3_disco`` with your board:

 .. zephyr-app-commands::
    :zephyr-app: samples/boards/stm32/uart/single_wire
    :board: stm32f3_disco
    :goals: build flash
    :compact:

After flashing the console output should not show any failure reports,
but the following message repeated every 2s:

.. code-block:: none

    Received c
