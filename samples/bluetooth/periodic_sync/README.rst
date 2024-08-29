.. zephyr:code-sample:: ble_periodic_adv_sync
   :name: Periodic Advertising Synchronization
   :relevant-api: bt_gap bluetooth

   Use BLE Periodic Advertising Synchronization functionality.

Overview
********

A simple application demonstrating the BLE Periodic Advertising Synchronization
functionality.

Requirements
************

* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_sync` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv` on
another board that will start periodic advertising, to which this sample will
establish periodic advertising synchronization.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
