.. zephyr:code-sample:: ble_central_otc
   :name: Central OTC
   :relevant-api: bluetooth

   Connect to a Bluetooth LE peripheral that supports the Object Transfer Service (OTS)

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application specifically looks for the OTS (Object Transfer) GATT Service.
And this sample is to select object sequentially, to read metadata, to write data,
to read data, and to calculate checksum of selected objects.

Requirements
************

* A board with Bluetooth LE support and 4 buttons.

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/central_otc` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
