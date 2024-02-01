.. _bluetooth-hci-usb-sample:

Bluetooth: HCI USB
##################

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

Notes on HCI over USB
*********************

USB is a serial protocol. However, the implementation of the peripheral can
expose multiple endpoints, in effect re-ordering serial data into per-endpoint
receive queues.

The Bluetooth specification defines separate endpoints per HCI packet type.

As the HCI protocol was not originally designed with re-ordering in mind, using
this transport may expose the application to deadlocks and general unexpected
behavior.

Because of this, the industry-standard way to expose a Bluetooth Controller over
USB is now to use the H4 HCI transport over virtual serial (i.e. CDC class).

See the :ref:`HCI UART sample <bluetooth-hci-uart-sample>` documentation for
more details.

If using the controller with a BlueZ host, one can also use the USB HCI H4
transport. It exposes the H4 HCI transport, but over two custom bulk endpoints.

See the :ref:`HCI H4 over USB sample <bluetooth-hci-usb-h4-sample>` for more
details.
