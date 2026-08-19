.. zephyr:code-sample:: ble_periodic_adv
   :name: Periodic Advertising
   :relevant-api: bt_gap bluetooth

   Use Bluetooth LE Periodic Advertising functionality.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising functionality.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_adv
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will start periodic advertising, periodically updating the
manufacturer data counter.

Use the :zephyr:code-sample:`ble_periodic_adv_sync` sample on a second board to scan
and establish a periodic advertising synchronization to this device.
