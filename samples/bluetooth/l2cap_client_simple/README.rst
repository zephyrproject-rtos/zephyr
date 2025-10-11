.. _bluetooth_l2cap_server_simple:

L2CAP Server Simple Sample
###########################

Overview
********

This sample demonstrates how to create a simple L2CAP (Logical Link Control and
Adaptation Protocol) server that listens for incoming L2CAP connections on a
fixed PSM (Protocol/Service Multiplexer). When a client connects, the server
accepts the connection and receives data sent over the L2CAP channel.

The sample uses a fixed allocation strategy where one L2CAP channel instance is
pre-allocated for each possible Bluetooth connection (based on
``CONFIG_BT_MAX_CONN``). This approach avoids dynamic memory allocation and
simplifies resource management.

Requirements
************

* A board with Bluetooth Low Energy (BLE) support
* Bluetooth controller with L2CAP Connection Oriented Channels support

Building and Running
********************

This sample can be built and run on any board with Bluetooth LE support.

To build the sample for the nRF52840 DK:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_server_simple
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

For testing with native_posix (simulation):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_server_simple
   :board: native_posix
   :goals: build
   :compact:

After flashing, the device will register an L2CAP server on PSM 0x29 and wait
for incoming connections. Pair this sample with the
:zephyr:code-sample:`bluetooth_l2cap_client_simple` sample to establish a
connection and exchange data.

Sample Output
=============

Upon successful initialization:

.. code-block:: console

   L2CAP server registered, PSM 41

When a client connects:

.. code-block:: console

   L2CAP: accepted connection, assigned chan[0]
   L2CAP connected

When data is received:

.. code-block:: console

   L2CAP received 12 bytes

Testing
*******

This sample is designed to work with the
:zephyr:code-sample:`bluetooth_l2cap_client_simple` sample. Run the server
sample on one device and the client sample on another device within BLE range.

The client will scan, connect, and establish an L2CAP channel to send periodic
messages to the server.

References
**********

* :ref:`bluetooth_api`
* :ref:`bluetooth-samples`
* `Bluetooth Core Specification v5.4, Vol 3, Part A (L2CAP)
  <https://www.bluetooth.com/specifications/specs/core-specification-5-4/>`_