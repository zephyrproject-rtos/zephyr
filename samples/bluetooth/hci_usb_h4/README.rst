.. _bluetooth-hci-usb-h4-sample:

Bluetooth: HCI H4 over USB
##########################

Overview
********

Make a USB H4 Bluetooth dongle out of Zephyr. Requires USB device support from
the board it runs on (e.g. :ref:`nrf52840dk_nrf52840` supports both BLE and
USB).

Requirements
************

* Bluetooth stack running on the host (e.g. BlueZ)
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/hci_usb_h4` in
the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
