.. _bluetooth_l2cap_client_simple:

L2CAP Client Simple Sample
###########################

Overview
********

This sample demonstrates how to create a simple L2CAP (Logical Link Control and
Adaptation Protocol) client that scans for Bluetooth devices, connects to one,
and establishes an L2CAP Connection Oriented Channel to communicate with a
server.

The sample periodically sends short text messages over the L2CAP channel to
demonstrate basic data transmission. It uses a fixed allocation strategy for
the L2CAP channel to avoid dynamic memory allocation.

Requirements
************

* A board with Bluetooth Low Energy (BLE) support
* Bluetooth controller with L2CAP Connection Oriented Channels support
* A peer device running an L2CAP server (such as the
  :zephyr:code-sample:`bluetooth_l2cap_server_simple` sample)

Building and Running
********************

This sample can be built and run on any board with Bluetooth LE support.

To build the sample for the nRF52840 DK:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_client_simple
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

For testing with native_posix (simulation):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/l2cap_client_simple
   :board: native_posix
   :goals: build
   :compact:

After flashing, the device will scan for nearby Bluetooth devices, connect to
the first one it finds, and attempt to establish an L2CAP channel on PSM 0x29.
Once connected, it will send messages periodically.

Sample Output
=============

Upon successful connection and channel establishment:

.. code-block:: console

   Scanning...
   Connected
   L2CAP channel connected
   Sent: Hello from client

The message "Hello from client" (or similar) will be sent periodically to
demonstrate ongoing data transmission.

Testing
*******

This sample is designed to work with the
:zephyr:code-sample:`bluetooth_l2cap_server_simple` sample:

1. Flash the server sample on one device
2. Flash the client sample on another device
3. Place both devices within BLE range
4. The client will automatically scan, connect, and establish the L2CAP channel
5. Monitor the serial output on both devices to observe the connection and data
   exchange

Notes
*****

* This sample is intentionally simple and is designed for testing and
  demonstration purposes
* The client connects to the first advertising device it finds - in a
  production environment, you would want to filter devices by name, address, or
  advertised services
* You may need to adjust Bluetooth settings in the ``prj.conf`` file to match
  your platform's capabilities

References
**********

* :ref:`bluetooth_api`
* :ref:`bluetooth-samples`
* `Bluetooth Core Specification v5.4, Vol 3, Part A (L2CAP)
  <https://www.bluetooth.com/specifications/specs/core-specification-5-4/>`_