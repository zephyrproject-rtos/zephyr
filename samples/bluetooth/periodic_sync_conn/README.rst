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

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_sync_conn` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv_conn` on
another board that will start periodic advertising and connect to this sample
once synced.

See :zephyr:code-sample-category:`bluetooth` samples for details.
