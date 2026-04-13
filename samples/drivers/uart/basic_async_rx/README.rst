.. zephyr:code-sample:: uart-async-rx_test
   :name: UART Async RX Test
   :relevant-api: uart_interface

   A simple example that demonstrates asynchronous UART receiving.

Overview
********

This sample application demonstrates how to use the UART driver's asynchronous
TX API to receive data. It receives a test message through UART1 (PA18) every 2
seconds using the Zephyr asynchronous UART API.

The application registers a callback that handles UART events such as RX_RDY, RX_BUF_REQUEST
and RX_BUF_RELEASED, showing proper usage of the async RX API.

The application is specifically designed for the TI MSPM0G3507 LaunchPad,
but can be adapted to work with other boards by modifying the overlay file.

Building and Running
*******************

Build and flash the sample for the MSPM0G3507 LaunchPad:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/basic_async_rx
   :board: lp_mspm0g3507
   :goals: build flash
   :compact:

This examples works hand in hand with the basic_async_tx example and requires 2 launchpads
for the setup.

Board 1: TX - runs basic_async_tx
Board 2: RX - runs basic_async_rx

Connect PA17 of Board 1 to PA18 of Board 2. Connect GND of both boards together.
To observe the output on the console, connect to UART0 (PA10/PA11). To observe the
received data, connect a logic analyzer or oscilloscope to UART1 RX (PA18).

Sample Output
************

.. code-block:: console

   Test 1
   Test 2
   Test 3
   .
   .
   .
   .
