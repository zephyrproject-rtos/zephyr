.. _bluetooth-observer-sample:

Bluetooth: Observer
###########################

Overview
********

A simple application demonstrating Bluetooth Low Energy Observer role functionality
The application will periodically scan for devices nearby. If any found,
prints the address of the device and the RSSI value to the UART terminal.

Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/observer` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
