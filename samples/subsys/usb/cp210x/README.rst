.. zephyr:code-sample:: usb-cp210x
   :name: USB CP210x UART sample
   :relevant-api: usbd_api uart_interface

   Use USB CP210x UART driver to implement a serial port echo.

Overview
********

This sample app demonstrates use of a USB CP210x virtual COM port
driver provided by the Zephyr project.
Received data from the serial port is echoed back to the same port
provided by this driver.
This sample can be found under :zephyr_file:`samples/subsys/usb/cp210x` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver, which is available for multiple
boards supported in Zephyr.
The operating system needs to be configured to bind PID-VID pairs to the
appropriate driver. For example, on Linux, to bind PID 0x2fe3 VID 0x0002
to the cp210x driver, one needs to create a file
file:`/etc/udev/rules.d/99-zephyr-cp210x.rules` with the following lines

.. code-block:: udev

   ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="2fe3", ATTRS{idProduct}=="0002", \
   RUN+="/sbin/modprobe cp210x", \
   RUN+="/bin/sh -c 'echo 2fe3 0002 > /sys/bus/usb-serial/drivers/cp210x/new_id'"


Building and Running
********************

Reel Board
===========

To see the console output of the app, open a serial port emulator and
attach it to the USB to TTL Serial cable. Build and flash the project:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/cdc_acm
   :board: reel_board
   :goals: flash
   :compact:

Running
=======

Plug the board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

   usb 1-1: new full-speed USB device number 43 using xhci_hcd
   usb 1-1: New USB device found, idVendor=1ba4, idProduct=0002, bcdDevice= 4.02
   usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 1-1: Product: CP210X serial backend
   usb 1-1: Manufacturer: Zephyr Project
   usb 1-1: SerialNumber: 000000000000000000000000
   cp210x 1-1:1.0: cp210x converter detected
   usb 1-1: cp210x converter now attached to ttyUSB1

The app prints on serial output (UART1), used for the console:

.. code-block:: console

   Wait for DTR

Open a serial port emulator, for example minicom
and attach it to detected CP210x device:

.. code-block:: console

   minicom --device /dev/ttyUSB1

The app should respond on serial output with:

.. code-block:: console

   DTR set, start test
   Baudrate detected: 115200

And on ttyUSB device, provided by zephyr USB device stack:

.. code-block:: console

   Send characters to the UART device
   Characters read:

The characters entered in serial port emulator will be echoed back.
