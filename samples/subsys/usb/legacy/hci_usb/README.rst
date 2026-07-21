.. zephyr:code-sample:: legacy_bluetooth_hci_usb
   :name: Legacy HCI USB
   :relevant-api: hci_raw bluetooth _usb_device_core_api

   Turn a Zephyr board into a USB Bluetooth dongle (compatible with all operating systems).

Overview
********

Make a USB Bluetooth dongle out of Zephyr.

Requirements
************

* Bluetooth stack running on the host (e.g. BlueZ)
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************
This sample can be found under :zephyr_file:`samples/subsys/usb/legacy/hci_usb` in the
Zephyr tree.
