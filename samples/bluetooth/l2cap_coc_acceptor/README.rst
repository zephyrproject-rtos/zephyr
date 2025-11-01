.. zephyr:code-sample:: bluetooth_l2cap_coc_acceptor
   :name: L2CAP Connection Oriented Channels (Acceptor)
   :relevant-api: bt_l2cap

   Accept incoming L2CAP Connection Oriented Channel connections.

Overview
********

This sample demonstrates how to create an L2CAP Connection Oriented Channel
(CoC) server that listens for incoming L2CAP connections on a dynamic PSM
(Protocol/Service Multiplexer). When a client connects, the server accepts
the connection and receives data sent over the L2CAP channel.

The sample uses a fixed allocation strategy where one L2CAP channel instance is
pre-allocated for each possible Bluetooth connection (based on
``CONFIG_BT_MAX_CONN``). This approach avoids dynamic memory allocation and
simplifies resource management.

Requirements
************

* A board with Bluetooth Low Energy (BLE) support

Building and Running
********************

This sample can be built and run on any board with Bluetooth LE support.

To build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_coc_acceptor
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

After flashing, the device will register an L2CAP server on PSM 0x29 and wait
for incoming connections. Pair this sample with the
:zephyr:code-sample:`bluetooth_l2cap_coc_initiator` sample to establish a
connection and exchange data.

The initiator will automatically scan, connect, and establish the L2CAP channel.
Monitor the serial output on both devices to observe the connection and data
exchange.

Sample Output
=============

Upon successful initialization:

.. code-block:: console

   L2CAP server registered, PSM 41

When a client connects:

.. code-block:: console

   L2CAP channel accepted, assigned chan[0]
   L2CAP channel connected

When data is received:

.. code-block:: console

   L2CAP channel received 17 bytes
