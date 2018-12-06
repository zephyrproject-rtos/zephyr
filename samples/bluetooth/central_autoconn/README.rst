.. _bluetooth_central_autoconn:

Bluetooth: Central Autoconnect
##################

Overview
********

Application demonstrating BLE Central role with whitelist based autoconnect
functionality by scanning for other BLE devices and establishing a connection
to the first one with a strong enough signal. Autoconnect is enabled so that if
link disconnected controller will try to reconnect to this device.



Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************
This sample can be found under :file:`samples/bluetooth/central_autoconn` in the
Zephyr tree.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.
