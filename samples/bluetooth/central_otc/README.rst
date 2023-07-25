.. _central_otc:

Bluetooth: Central OTC
######################

Overview
********

Similar to the :ref:`Central <bluetooth_central>` sample, except that this
application specifically looks for the OTS (Object Transfer) GATT Service.
And this sample is to select object sequentially, to read metadata, to write data,
to read data, and to calculate checksum of selected ojects.

Requirements
************

* A board with BLE support and 4 buttons.

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/central_otc` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
