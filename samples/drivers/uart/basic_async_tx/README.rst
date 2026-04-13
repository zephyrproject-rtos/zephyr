.. zephyr:code-sample:: uart-async-tx-test
   :name: UART Async TX Test
   :relevant-api: uart_interface

   A simple example that demonstrates asynchronous UART transmission.

Overview
********

This sample application demonstrates how to use the UART driver's asynchronous
TX API to transmit data. It sends a test message through UART1 (PA17) every 2
seconds using the Zephyr asynchronous UART API.

The application registers a callback that handles UART events such as TX_DONE
and TX_ABORTED, showing proper usage of the async TX API. Status information
is printed to the console via UART0.

The application is specifically designed for the TI MSPM0G3507 LaunchPad,
but can be adapted to work with other boards by modifying the overlay file.

Building and Running
*******************

Build and flash the sample for the MSPM0G3507 LaunchPad:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/basic_async_tx
   :board: lp_mspm0g3507
   :goals: build flash
   :compact:

This examples works hand in hand with the basic_async_rx example and requires 2 launchpads
for the setup.

Board 1: TX - runs basic_async_tx
Board 2: RX - runs basic_async_rx

Connect PA17 of Board 1 to PA18 of Board 2. Connect GND of both boards together.
To observe the output on the console, connect to UART0 (PA10/PA11). To observe the
transmitted data, connect a logic analyzer or oscilloscope to UART1 TX (PA17).

Sample Output
************

.. code-block:: console

   ===================================
   Asynchronous TX Test
   Transmitting on PA17 (UART1 TX)
   ===================================

   Callback registered successfully
   Started async TX of 16 bytes
   TX completed - sent 16 bytes
   Started async TX of 16 bytes
   TX completed - sent 16 bytes
   Started async TX of 16 bytes
   TX completed - sent 16 bytes
   ...
