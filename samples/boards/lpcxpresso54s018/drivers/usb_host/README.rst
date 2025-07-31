.. _usb_host_sample:

USB Host Sample
###############

Overview
********

This sample demonstrates USB host functionality on the LPC54S018 board.
It shows how to detect USB device connection/disconnection and enumerate
connected devices.

The sample uses the IP3516HS USB host controller driver that's built into
Zephyr for NXP devices.

Requirements
************

- LPC54S018 board
- USB device to connect (e.g., USB flash drive, keyboard, mouse)
- USB OTG cable or adapter

Building and Running
********************

Build and flash this sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/usb_host
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

After flashing, connect a USB device to the USB1 port. The application will:

1. Initialize the USB host stack
2. Enable VBUS to power the connected device
3. Detect when a USB device is connected or disconnected
4. Display device information (VID, PID, device class)

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0 ***
   [00:00:00.010,000] <inf> usb_host_sample: USB Host sample application
   [00:00:00.010,000] <inf> usb_host_sample: Waiting for USB device to be connected...
   [00:00:00.020,000] <inf> usb_host_sample: USB host initialized successfully
   [00:00:05.123,000] <inf> usb_host_sample: USB device connected
   [00:00:05.123,000] <inf> usb_host_sample:   VID: 0x0781, PID: 0x5567
   [00:00:05.123,000] <inf> usb_host_sample:   Device class: 0x08

Shell Commands
==============

When CONFIG_USBH_SHELL is enabled, you can use the following shell commands:

- ``usbh status`` - Show USB host status
- ``usbh list`` - List connected USB devices
- ``usbh tree`` - Show USB device tree

References
**********

- :ref:`usb_api`
- `LPC54S018 Reference Manual`_

.. _LPC54S018 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/LPC54S01XJ2J4RM.pdf