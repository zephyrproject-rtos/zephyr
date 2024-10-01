.. zephyr:code-sample:: ble_central_gatt_write
   :name: Central / GATT Write
   :relevant-api: bluetooth

   Scan for a Bluetooth LE device, connect to it and write a value to a characteristic.

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application use GATT Write Without Response.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/central_gatt_write`
in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
