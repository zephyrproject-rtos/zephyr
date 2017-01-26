Bluetooth: HCI UART
####################

Overview
*********

Expose Zephyr Bluetooth Controller support over UART to another device/CPU using
the H:4 HCI transport protocol (requires HW flow control from the UART)



Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************
This sample can be found under :file:`samples/bluetooth/hci_uart` in the
Zephyr tree.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.
