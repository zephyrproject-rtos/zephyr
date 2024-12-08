.. zephyr:code-sample:: ble_periodic_adv_sync
   :name: Periodic Advertising Synchronization
   :relevant-api: bt_gap bluetooth

   Use Bluetooth LE Periodic Advertising Synchronization functionality.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising Synchronization
functionality.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_sync` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv` on
another board that will start periodic advertising, to which this sample will
establish periodic advertising synchronization.

See :zephyr:code-sample-category:`bluetooth` samples for details.
