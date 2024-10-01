.. zephyr:code-sample:: ble_scan_adv
   :name: Scan & Advertise
   :relevant-api: bt_gap bluetooth

   Combine Bluetooth LE Broadcaster & Observer roles to advertise and scan for devices simultaneously.

Overview
********

A simple application demonstrating combined Bluetooth LE Broadcaster & Observer
role functionality. The application will periodically send out
advertising packets with a manufacturer data element. The content of the
data is a single byte indicating how many advertising packets the device
has received (the number will roll back to 0 after 255).

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/scan_adv` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
