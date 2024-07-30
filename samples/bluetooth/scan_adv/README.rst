.. _bluetooth-scan-adv-sample:

Bluetooth: Scan & Advertise
###########################

Overview
********

A simple application demonstrating combined BLE Broadcaster & Observer
role functionality. The application will periodically send out
advertising packets with a manufacturer data element. The content of the
data is a single byte indicating how many advertising packets the device
has received (the number will roll back to 0 after 255).

Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/scan_adv` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
