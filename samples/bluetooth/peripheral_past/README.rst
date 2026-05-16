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

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_past
   :board: <board>
   :goals: build flash
   :compact:

You can use the :zephyr:code-sample:`ble_central_past` sample on another board that
will connect to this device and transfer a periodic advertising sync.

See :zephyr:code-sample-category:`bluetooth` samples for details.
