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

The application initiates a passive scan to detect nearby Bluetooth LE devices.
It specifically looks for devices that have a signal strength greater
than -50dBm. This threshold helps the app filter out weaker signals,
ensuring it only interacts with devices that are within a reasonable RSSI
range for communication.

Connection handling
===================

1. The Central scans for Peripheral devices and if it finds a Peripheral
   which has a signal strength higher than -50dBm, an attempt to establish
   LE connection is made.
2. If the connection is successful, the Central initiates disconnect to
   the Peripheral and then restarts the scan.
3. If there are no connections, the Central keeps scanning continuously.

The sample is used to demonstrate the Central mode capabilities of Bluetooth LE and
hence a disconnect is issued right immediately after establishing a connection with
a Peripheral, allowing the Central to resume scanning for other devices.

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

To test Central's scanning functionality, either flash the :zephyr:code-sample:`ble_peripheral`
sample on a second compatible board or use an off-the-shelf Bluetooth LE enabled
device that can act as a Peripheral (eg. smartphone, smartwatch, etc.).
