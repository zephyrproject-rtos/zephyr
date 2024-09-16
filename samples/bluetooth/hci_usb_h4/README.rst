.. zephyr:code-sample:: bluetooth_hci_usb_h4
   :name: HCI H4 over USB
   :relevant-api: hci_raw bluetooth _usb_device_core_api usbd_api

   Turn a Zephyr board into a USB H4 Bluetooth dongle (Linux/BlueZ only).

Overview
********

Make a USB H4 Bluetooth dongle out of Zephyr. Requires USB device support from
the board it runs on (e.g. :ref:`nrf52840dk_nrf52840` supports both Bluetooth LE and
USB).

Requirements
************

* Bluetooth stack running on the host (e.g. BlueZ)
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/hci_usb_h4` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
