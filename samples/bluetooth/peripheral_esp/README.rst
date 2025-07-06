.. zephyr:code-sample:: ble_peripheral_esp
   :name: ESP Peripheral
   :relevant-api: bt_gatt bt_bas bluetooth

   Expose environmental information using the Environmental Sensing Profile (ESP).

Overview
********
Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the ESP (Environmental Sensing Profile) GATT
Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_esp` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
