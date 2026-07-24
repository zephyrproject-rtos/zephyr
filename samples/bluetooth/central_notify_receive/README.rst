.. zephyr:code-sample:: ble_central_notify_receive
   :name: Central / Notify Receive
   :relevant-api: bluetooth

   Scan for a Bluetooth LE peripheral and receive GATT notifications.

Overview
********

This sample acts as a Bluetooth LE central that scans for a peripheral
advertising the custom notify service, connects to it, discovers the notify
characteristic, subscribes to notifications, and measures the received
notification throughput. Throughput metrics are printed to the console at a
configurable interval.

This sample is designed to work with the
:zephyr:code-sample:`ble_central_peripheral_notify_send_receive` sample which
acts as a multirole device that sends notifications to this central.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_notify_receive
   :board: <board>
   :goals: build flash
   :compact:
