.. zephyr:code-sample:: ble_peripheral_dis
   :name: DIS Peripheral
   :relevant-api: bt_gatt bluetooth

   Expose device information using the Device Information Service (DIS).

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the DIS (Device Information) GATT Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_dis` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
