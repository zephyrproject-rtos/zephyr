.. zephyr:code-sample:: ble_central
   :name: Central
   :relevant-api: bluetooth

   Implement basic Bluetooth LE Central role functionality (scanning and connecting).

Overview
********

Application demonstrating very basic Bluetooth LE Central role functionality by scanning
for other Bluetooth LE devices and establishing a connection to the first one with a
strong enough signal.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/central` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
