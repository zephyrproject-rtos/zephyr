.. zephyr:code-sample:: ble_periodic_adv_conn
   :name: Periodic Advertising Connection Procedure (Initiator)
   :relevant-api: bt_gap bluetooth

   Initiate a connection to a device using the Periodic Advertising Connection Procedure.

Overview
********

A simple application demonstrating the initiator side of the Bluetooth LE
Periodic Advertising Connection Procedure.

Before starting PAwR advertising, this sample first scans for the responder
(:zephyr:code-sample:`ble_periodic_adv_sync_conn`) by name, connects to it over
a regular connection, and stores its address. It then disconnects and starts
PAwR advertising.

When the responder replies to a subevent, this sample uses the address it
stored earlier (rather than parsing it out of the response data) to connect
to the responder using the Periodic Advertising Connection Procedure. Once
the connection is established, it will wait for disconnect before connecting
again on a subsequent response.

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

Start the :zephyr:code-sample:`ble_periodic_adv_sync_conn` sample on a second board
first. After flashing, this device will scan for and connect to the responder to learn
its address, then disconnect and start PAwR advertising. Once the responder replies to
a subevent, the initiator will connect to it using the address learned earlier. After
disconnection, it waits for the next response to establish a new connection.

Use the :zephyr:code-sample:`ble_periodic_adv_sync_conn` sample on a second board to
synchronize and respond to this device.
