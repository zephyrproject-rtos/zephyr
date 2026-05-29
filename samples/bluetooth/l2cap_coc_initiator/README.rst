.. zephyr:code-sample:: bluetooth_l2cap_coc_initiator
   :name: L2CAP Connection Oriented Channels (Initiator)
   :relevant-api: bt_l2cap

   Initiate L2CAP Connection Oriented Channel connections and send data.

Overview
********

This sample demonstrates how to create an L2CAP Connection Oriented Channel
(CoC) client that scans for Bluetooth devices, connects to one, and establishes
an L2CAP channel to communicate with a server.

The sample periodically sends short text messages over the L2CAP channel to
demonstrate basic data transmission.

Requirements
************

* A board with Bluetooth Low Energy (BLE) support
* A peer device running an L2CAP server (such as the
  :zephyr:code-sample:`bluetooth_l2cap_coc_acceptor` sample)

Building and Running
********************

This sample can be built and run on any board with Bluetooth LE support.

To build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_coc_initiator
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

After flashing, the device will scan for nearby Bluetooth devices, connect to
the first one it finds, and attempt to establish an L2CAP channel on PSM 0x29.

Pair this sample with the :zephyr:code-sample:`bluetooth_l2cap_coc_acceptor`
sample running on another device. The initiator will automatically scan, connect,
and establish the L2CAP channel. Monitor the serial output on both devices to
observe the connection and data exchange.

Sample Output
=============

Upon successful connection and channel establishment:

.. code-block:: console

   Scanning...
   Found device: XX:XX:XX:XX:XX:XX (RSSI -45)
   Connecting to XX:XX:XX:XX:XX:XX ...
   Connected
   L2CAP channel connection initiated
   L2CAP channel connected
   Sent: Hello from client

The message "Hello from client" will be sent periodically.
