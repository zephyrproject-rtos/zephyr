.. zephyr:code-sample:: ble_periodic_adv_sync_conn
   :name: Periodic Advertising Connection Procedure (Responder)
   :relevant-api: bt_gap bluetooth

   Respond to periodic advertising and establish a connection.

Overview
********

A simple application demonstrating the responder side of the Bluetooth LE
Periodic Advertising Connection Procedure.

This sample will send its address in response to the advertiser when receiving
subevent data. Once the connection is established, it will disconnect and wait
for a new connection to be established.

Requirements
************

* A board with Bluetooth LE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Scanner feature

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_sync_conn
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will scan for a PAwR advertiser, synchronize to it, and
respond with its own address. Once the initiator connects, this device will disconnect
and wait for a new connection.

Use the :zephyr:code-sample:`ble_periodic_adv_conn` sample on a second board to start
PAwR advertising and connect to this device once synced.
