.. zephyr:code-sample:: ble_periodic_adv_conn
   :name: Periodic Advertising Connection Procedure (Initiator)
   :relevant-api: bt_gap bluetooth

   Initiate a connection to a device using the Periodic Advertising Connection Procedure.

Overview
********

A simple application demonstrating the initiator side of the BLE
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

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_adv_conn` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_sync_conn` in the
Zephyr tree that will synchronize and respond to this sample.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
