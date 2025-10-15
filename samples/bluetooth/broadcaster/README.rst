.. zephyr:code-sample:: bluetooth_broadcaster
   :name: Broadcaster
   :relevant-api: bluetooth

   Periodically send out advertising packets with a manufacturer data element.

Overview
********

A simple application demonstrating Bluetooth Low Energy Broadcaster role functionality.
The application will periodically send out advertising packets with
a manufacturer data element. The content of the data is a single byte
indicating how many advertising packets the device has sent
(the number will roll back to 0 after 255).

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

See :zephyr:code-sample-category:`bluetooth` samples for details.
