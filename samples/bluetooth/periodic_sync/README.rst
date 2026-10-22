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

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_sync
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will scan for periodic advertisers and establish a
periodic advertising synchronization.

Use the :zephyr:code-sample:`ble_periodic_adv` sample on a second board to start
periodic advertising, to which this device will synchronize.
