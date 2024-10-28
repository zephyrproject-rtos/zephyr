.. zephyr:code-sample:: ble_central_past
   :name: Central Periodic Advertising Sync Transfer (PAST)
   :relevant-api: bt_gap bluetooth

   Use the Periodic Advertising Sync Transfer (PAST) feature as the sender.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising Sync Transfer
functionality as the sender.

Requirements
************

* A board with Bluetooth LE 5.1 support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/central_past` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv` on
another board that will start periodic advertising, to which this sample will
establish periodic advertising synchronization.

Use the sample found under :zephyr_file:`samples/bluetooth/peripheral_past` in
the Zephyr tree on another board that will advertise and await a periodic
advertising sync transfer.

See :zephyr:code-sample-category:`bluetooth` samples for details.
