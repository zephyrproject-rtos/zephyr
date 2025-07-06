.. zephyr:code-sample:: usb-cdc-acm-console
   :name: Console over USB CDC ACM
   :relevant-api: _usb_device_core_api usbd_api

   Output "Hello World!" to the console over USB CDC ACM.

Overview
********

A simple Hello World sample, with console output coming via CDC ACM UART.
Primarily intended to show the required config options.

Requirements
************

This project requires a USB device controller driver.

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
The board will be detected as a CDC_ACM serial device. To see the console output
from the sample, use a command similar to :command:`minicom -D /dev/ttyACM0`.

.. code-block:: console

   Hello World! arm
   Hello World! arm
   Hello World! arm
   Hello World! arm

Troubleshooting
===============

You may need to stop :program:`modemmanager` via :command:`sudo stop modemmanager`, if it is
trying to access the device in the background.
