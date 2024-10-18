.. zephyr:code-sample:: usbd-dfu
   :name: USB DFU
   :relevant-api: usbd_api

   Implement a basic USB DFU device

Overview
********

This sample application demonstrates the USB DFU implementation using the
new experimental USB device stack.

Requirements
************

This project requires an experimental USB device driver (UDC API) and uses the
:ref:`disk_access_interface` API and RAM-disk to download/upload the image.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nRF52840DK board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/usbd-dfu
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:
