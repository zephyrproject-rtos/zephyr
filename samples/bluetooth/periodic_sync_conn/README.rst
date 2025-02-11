.. _bluetooth-periodic-advertising-sync-conn-sample:

Bluetooth: Periodic Advertising Connection Procedure - Responder
#####################################################################

Overview
********

A simple application demonstrating the responder side of the BLE
Periodic Advertising Connection Procedure.

This sample will send its address in response to the advertiser when receiving
subevent data. Once the connection is established, it will disconnect and wait
for a new connection to be established.

Requirements
************

* A board with BLE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Scanner feature

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_sync_conn` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv_conn` on
another board that will start periodic advertising and connect to this sample
once synced.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
