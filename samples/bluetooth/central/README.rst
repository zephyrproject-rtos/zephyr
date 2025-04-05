.. zephyr:code-sample:: ble_central
   :name: Central
   :relevant-api: bluetooth

   Implement basic Bluetooth LE Central role functionality
   (scanning and connecting).

Overview
********

This application demonstrates basic Bluetooth LE Central role functionality
by scanning for other Bluetooth LE devices and establishing a connection
to the first one with a strong enough signal.

Core features
*************

Scanning for devices
====================

The application initiates a passive scan to detect nearby BLE devices.
It specifically looks for devices that have a signal strength greater
than -50dBm.This threshold helps the app filter out weaker signals,
ensuring it only interacts with devices that are within a reasonable
range for communication.

Connection handling
===================

1. The central scans for peripheral devices and if it finds a peripheral
   which has a signal strength higher than -50dBm, an attempt to establish
   LE connection is made.
2. If the connection is successful, the central initiates disconnect to
   the peripheral and then restarts the scan.
3. If there are no connections, the central keeps scanning continuously.

The sample is used to demonstrate the Central mode capabilities of BLE and
hence a disconnect is issued right after connection with a peripheral,
to keep the central in scan mode.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and running
********************

Build and flash the sample as follows, replacing board_name with your
target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central
   :board: board_name
   :goals: build flash
   :compact:

To test Central's scanning functionality, either flash the peripheral
sample on a second compatible board or use an off-the-shelf BLE enabled
device that can act as a peripheral (eg. smartphone, smartwatch, etc.).

Adding support for new boards:
******************************

To add Bluetooth Central support for a new board:

* Ensure that the following BLE configurations are enabled in prj.conf

  .. code-block:: cfg

     # Enable BLE
     CONFIG_BT=y
     # Enable central role
     CONFIG_BT_CENTRAL=y
