.. zephyr:code-sample:: ble_central_multilink
   :name: Central Multilink
   :relevant-api: bluetooth

   Scan, connect and establish connection to up to 62 peripherals.

Overview
********

Application demonstrating Bluetooth LE Central role functionality by scanning for other
Bluetooth LE devices and establishing connection to up to 62 peripherals with a strong
enough signal.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_multilink
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample scans for and connects to Bluetooth LE peripherals within
close range. Run any Bluetooth LE peripheral sample on one or more additional boards
to provide peers for the central to connect to.
