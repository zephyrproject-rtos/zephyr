.. zephyr:code-sample:: bflb-bl61x-wo-uart
   :name: Bouffalolab GPIO FIFO / Wire Out UART
   :relevant-api: gpio_bl61x_wo_interface

   Output UART using Wire Out

Overview
********

A sample application that demonstrates usage of Bouffalolab GPIO FIFO / Wire Out to output
custom protocols by implementing UART.
You can experiment with this sample by making 16 pins output the string together, changing
the baudrate, amount of stop bits, or even the polarities, etc.

Requirements
************

Tie the output pins specified in the app.overlay to the board's default RX pin,
it will log the message sent.
Alternatively, plug an UART adapter RX to the pin and set it up to receive 115200 8N1 UART.

Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/gpio/bflb_bl61x_wo_uart
   :board: ai_m61_32s_kit
   :goals: build


To build for another supported board, change "ai_m61_32s_kit" to that board's name.
All BL61x boards are supported.

Sample Output
=============

- On the board's regular UART:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-5197-g336bac14b121 ***
   Sent (0): Hello World!
   Sent (0): Hello World!
   Sent (0): Hello World!
   Sent (0): Hello World!

- At specified output pin:

.. code-block:: console

   Hello World!
   Hello World!
   Hello World!
   Hello World!
