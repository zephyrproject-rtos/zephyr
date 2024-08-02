.. _peripheral_hr:

Bluetooth: Peripheral HR
########################

Overview
********

Similar to the :ref:`Peripheral <ble_peripheral>` sample, except that this
application specifically exposes the HR (Heart Rate) GATT Service. Once a device
connects it will generate dummy heart-rate values.


Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_hr` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
