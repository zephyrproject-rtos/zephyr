.. zephyr:code-sample:: usb-cdc-acm-console
   :name: Console over USB CDC ACM
   :relevant-api: usbd_api uart_interface

   Output "Hello World!" to the console over USB CDC ACM.

Overview
********

This example application shows how to use the CDC ACM UART provided by the new
experimental USB device stack as a serial backend for the console.

Requirements
************

This project requires an experimental USB device driver (UDC API).

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the reel_board board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/console
   :board: reel_board
   :goals: flash
   :compact:

Plug the board into a host device, for sample, a PC running Linux OS.
The board will be detected as a CDC ACM serial device. To see the console output
from the sample, use a command similar to :command:`minicom -D /dev/ttyACM1`.

.. code-block:: console

   Hello World! arm
   Hello World! arm
   Hello World! arm
   Hello World! arm
