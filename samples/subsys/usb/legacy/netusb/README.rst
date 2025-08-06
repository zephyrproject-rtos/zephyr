.. zephyr:code-sample:: legacy-netusb
   :name: Legacy USB device netusb sample
   :relevant-api: net_config _usb_device_core_api

   Use USB CDC EEM/ECM or RNDIS implementation

Overview
********

Use the USB CDC EEM/ECM or RNDIS implementation to establish a network
connection to a device running the Zephyr RTOS.

.. note::
   This samples demonstrate deprecated :ref:`usb_device_stack`.

This sample can be found under :zephyr_file:`samples/subsys/usb/legacy/netusb` in the
Zephyr project tree.

Requirements
************

This project requires an USB device driver, which is available for multiple
boards supported in Zephyr.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/legacy/netusb
   :board: reel_board
   :goals: flash
   :compact:
