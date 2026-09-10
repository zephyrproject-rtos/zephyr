.. zephyr:code-sample:: ble_scan_adv
   :name: Scan & Advertise
   :relevant-api: bt_gap bluetooth

   Combine Bluetooth LE Broadcaster & Observer roles to advertise and scan for devices simultaneously.

Overview
********

A simple application demonstrating combined Bluetooth LE Broadcaster & Observer
role functionality. The application will periodically send out
advertising packets with a manufacturer data element. The content of the
data is a single byte indicating how many advertising packets the device
has received (the number will roll back to 0 after 255).

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/scan_adv
   :board: <board>
   :goals: build flash
   :compact:

The sample interleaves advertising and scanning for nearby Bluetooth LE devices. The manufacturer
data byte in the outgoing advertising packets reflects the number of advertising packets received.
Use a Bluetooth scanner app (e.g. nRF Connect) to observe the advertising packets and watch
the manufacturer data byte increment as the device receives packets from nearby devices.
