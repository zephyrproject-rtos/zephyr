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

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_past
   :board: <board>
   :goals: build flash
   :compact:

You can use the :zephyr:code-sample:`ble_periodic_adv` sample on another board that
will start periodic advertising, to which this sample will establish periodic
advertising synchronization.

Alternatively, you can use the :zephyr:code-sample:`ble_peripheral_past` sample on
another board that will advertise and await a periodic advertising sync transfer.

See :zephyr:code-sample-category:`bluetooth` samples for details.
