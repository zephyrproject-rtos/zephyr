.. zephyr:code-sample:: ble_peripheral_gatt_notify
   :name: Peripheral / GATT Notify
   :relevant-api: bluetooth

   Advertise and send GATT notifications as fast as possible to a connected central.

Overview
********

This sample acts as a Bluetooth LE peripheral that advertises, accepts a
connection from a central device, and then sends GATT notifications as fast as
possible. Throughput metrics are printed to the console at a configurable
interval.

This sample is designed to work together with the
:zephyr:code-sample:`ble_central_notify_receive` sample or the
:zephyr:code-sample:`ble_central_peripheral_notify_send_receive` sample acting
as the central.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_gatt_notify
   :board: <board>
   :goals: build flash
   :compact:
