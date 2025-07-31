.. zephyr:code-sample:: ble_peripheral
   :name: Peripheral
   :relevant-api: bt_gatt bluetooth

   Implement basic Bluetooth LE Peripheral role functionality (advertising and exposing GATT services).

Overview
********

Application demonstrating the Bluetooth LE Peripheral role. It has several well-known and
vendor-specific GATT services that it exposes.

It also implements user GATT GAP callbacks that allows controlling the name and appearance
of the device in a moment when the client tries to change them.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
