.. zephyr:code-sample:: bluetooth_hci_usb
   :name: HCI USB
   :relevant-api: hci_raw bluetooth usbd_api

   Turn a Zephyr board into a USB Bluetooth dongle (compatible with all operating systems).

Overview
********

Make a USB Bluetooth dongle out of Zephyr. Requires USB device support from the
board it runs on (e.g. :zephyr:board:`nrf52840dk` supports both Bluetooth LE and USB).

Requirements
************

* Bluetooth stack running on the host (e.g. BlueZ)
* A board with Bluetooth and USB support in Zephyr

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf52840dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_usb
   :board: <board>
   :goals: build flash
   :compact:
