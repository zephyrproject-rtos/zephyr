.. zephyr:code-sample:: legacy-usb-cdc-acm
   :name: Legacy USB CDC ACM UART sample
   :relevant-api: _usb_device_core_api uart_interface

   Use USB CDC ACM UART driver to implement a serial port echo.

Overview
********

This sample app demonstrates use of a USB Communication Device Class (CDC)
Abstract Control Model (ACM) driver provided by the Zephyr project.
Received data from the serial port is echoed back to the same port
provided by this driver.

.. note::
   This samples demonstrate deprecated :ref:`usb_device_stack`.

This sample can be found under :zephyr_file:`samples/subsys/usb/legacy/cdc_acm` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver, which is available for multiple
boards supported in Zephyr.

Building and Running
********************

Reel Board
===========

To see the console output of the app, open a serial port emulator and
attach it to the USB to TTL Serial cable. Build and flash the project:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/legacy/cdc_acm
   :board: reel_board
   :goals: flash
   :compact:

Running
=======

Plug the board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

   usb 9-1: new full-speed USB device number 112 using uhci_hcd
   usb 9-1: New USB device found, idVendor=8086, idProduct=f8a1
   usb 9-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 9-1: Product: CDC-ACM
   usb 9-1: Manufacturer: Intel
   usb 9-1: SerialNumber: 00.01
   cdc_acm 9-1:1.0: ttyACM1: USB ACM device

The app prints on serial output (UART1), used for the console:

.. code-block:: console

   Wait for DTR

Open a serial port emulator, for example minicom
and attach it to detected CDC ACM device:

.. code-block:: console

   minicom --device /dev/ttyACM1

The app should respond on serial output with:

.. code-block:: console

   DTR set, start test
   Baudrate detected: 115200

And on ttyACM device, provided by zephyr USB device stack:

.. code-block:: console

   Send characters to the UART device
   Characters read:

The characters entered in serial port emulator will be echoed back.

Troubleshooting
===============

If the ModemManager runs on your operating system, it will try
to access the CDC ACM device and maybe you can see several characters
including "AT" on the terminal attached to the CDC ACM device.
You can add or extend the udev rule for your board to inform
ModemManager to skip the CDC ACM device.
For this example, it would look like this:

.. code-block:: none

   ATTRS{idVendor}=="8086" ATTRS{idProduct}=="f8a1", ENV{ID_MM_DEVICE_IGNORE}="1"

You can use
``/lib/udev/rules.d/77-mm-usb-device-blacklist.rules`` as reference.
