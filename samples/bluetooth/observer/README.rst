.. zephyr:code-sample:: bluetooth_observer
   :name: Observer
   :relevant-api: bt_gap bluetooth

   Scan for Bluetooth devices nearby and print their information.

Overview
********

A simple application demonstrating Bluetooth Low Energy Observer role
functionality. The application will periodically scan for devices nearby.
If any found, prints the address of the device, the RSSI value, the Advertising
type, and the Advertising data length to the console.

If the used Bluetooth Low Energy Controller supports Extended Scanning, you may
enable :kconfig:option:`CONFIG_BT_EXT_ADV` in the project configuration file. Refer to the
project configuration file for further details.

Requirements
************

* A board with Bluetooth Low Energy support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/observer` in the
Zephyr tree.

See :ref:`Bluetooth samples section <bluetooth-samples>` for details.
