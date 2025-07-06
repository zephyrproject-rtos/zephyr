.. zephyr:code-sample:: ble_peripheral_past
   :name: Periodic Advertising Synchronization Transfer Peripheral
   :relevant-api: bt_gap bluetooth

   Use the Periodic Advertising Sync Transfer (PAST) feature as the receiver.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising Synchronization
Transfer (PAST) functionality as the receiver.

Requirements
************

* A board with Bluetooth LE 5.1 support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_past`
in the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/central_past` on
another board that will connect to this and transfer a periodic advertisement
sync.

See :zephyr:code-sample-category:`bluetooth` samples for details.
