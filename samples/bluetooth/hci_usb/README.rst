.. zephyr:code-sample:: bluetooth-hci-usb-sample
   :name: Bluetooth: HCI USB
   :relevant-api: hci_raw

   Expose a Bluetooth controller using the native USB HCI transport

Overview
********

Make a USB Bluetooth dongle out of Zephyr. Requires USB device support from the
board it runs on (e.g. :ref:`nrf52840dk_nrf52840` supports both BLE and USB).

Requirements
************

* Bluetooth stack running on the host (e.g. BlueZ)
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/hci_usb` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
