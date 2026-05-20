.. zephyr:code-sample:: ble_periodic_adv_conn
   :name: Periodic Advertising Connection Procedure (Initiator)
   :relevant-api: bt_gap bluetooth

   Initiate a connection to a device using the Periodic Advertising Connection Procedure.

Overview
********

A simple application demonstrating the initiator side of the Bluetooth LE
Periodic Advertising Connection Procedure.

How the initiator decides the address of the synced device to connect to
is application specific. This sample will connect to any synced device
responding with its address. Once the connection is established, it will
wait for disconnect before connecting to another synced device.

Requirements
************

* A board with Bluetooth LE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Advertiser feature

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_adv_conn
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will start PAwR advertising. Once a synced device responds
with its address, the initiator will connect to it. After disconnection, it waits for
the next response to establish a new connection.

Use the :zephyr:code-sample:`ble_periodic_adv_sync_conn` sample on a second board to
synchronize and respond to this device.
