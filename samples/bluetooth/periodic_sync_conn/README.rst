.. zephyr:code-sample:: ble_periodic_adv_sync_conn
   :name: Periodic Advertising Connection Procedure (Responder)
   :relevant-api: bt_gap bluetooth

   Respond to periodic advertising and establish a connection.

Overview
********

A simple application demonstrating the responder side of the Bluetooth LE
Periodic Advertising Connection Procedure.

This sample starts as a connectable advertiser so that the initiator
(:zephyr:code-sample:`ble_periodic_adv_conn`) can connect to it first over a
regular connection and learn its address. After the initiator disconnects,
this sample scans for the PAwR advertiser, synchronizes to it, and responds
to subevent data (with its device name, only to prove liveness, since the
initiator already knows its address). Once the initiator connects using the
Periodic Advertising Connection Procedure, this device will disconnect and
wait for a new connection to be established.

The responder uses its identity address so that the address learned during the
initial connection remains valid for the subsequent PAwR connection.

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

Start this sample first. After flashing, the device will advertise as connectable so
the initiator can connect and learn its address. Once the initiator disconnects, this
device will scan for the PAwR advertiser, synchronize to it, and respond to subevent
data. Once the initiator connects using the Periodic Advertising Connection Procedure,
this device will disconnect and wait for a new connection.

Use the :zephyr:code-sample:`ble_periodic_adv_conn` sample on a second board to connect
to this device, then start PAwR advertising and connect to it once synced.
