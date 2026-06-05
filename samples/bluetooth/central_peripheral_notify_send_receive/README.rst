.. zephyr:code-sample:: ble_central_peripheral_notify_send_receive
   :name: Central Peripheral / Notify Send Receive
   :relevant-api: bluetooth

   Multirole device that receives notifications as central and sends notifications as peripheral.

Overview
********

This sample acts as a Bluetooth LE multirole device supporting simultaneous
central and peripheral roles. As a central, it connects to a peripheral running
the :zephyr:code-sample:`ble_peripheral_gatt_notify` sample and receives GATT
notifications. As a peripheral, it advertises its own notify service and sends
GATT notifications to a connected central running the
:zephyr:code-sample:`ble_central_notify_receive` sample.

This creates a relay-like topology:

* **Device 1** (Central) ← notifications ← **Device 2** (Multirole) ← notifications ← **Device 3** (Peripheral)

Throughput metrics are printed to the console at a configurable interval for
both the received and sent notification streams.

Requirements
************

* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_peripheral_notify_send_receive
   :board: <board>
   :goals: build flash
   :compact:
