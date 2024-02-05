.. zephyr:code-sample:: bluetooth-hci-usb-h4-sample
   :name: Bluetooth: HCI USB-H4
   :relevant-api: hci_raw

   Expose a Bluetooth controller using the USB-H4 HCI transport

Overview
********

Make a USB H4 Bluetooth dongle out of Zephyr. Requires USB device support from
the board it runs on (e.g. :ref:`nrf52840dk_nrf52840` supports both BLE and
USB).

USB-H4 is not defined by the Bluetooth specification (at the time of writing
v5.3), but is supported by some host implementations, notably BlueZ on Linux.

Requirements
************

* Bluetooth stack running on the host that supports the USB H4 transport (e.g. BlueZ).
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/hci_usb_h4` in
the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
