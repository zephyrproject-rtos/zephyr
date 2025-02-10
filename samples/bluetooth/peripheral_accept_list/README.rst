.. zephyr:code-sample:: ble_peripheral_accept_list
   :name: Peripheral Accept List
   :relevant-api: bt_conn bt_gatt bluetooth

   Advertise and accept connections only from devices on an accept list.

Overview
********

This application demonstrates the Bluetooth LE advertising accept filter list feature.
If no device is bonded to the peripheral, casual advertising will be performed.
Once a device is bonded, on subsequent boots, connection requests will only be
accepted if the central device is on the accept list. Additionally, scan response
data will only be sent to devices that are on the accept list. As a result, some
Bluetooth LE central devices (such as Android smartphones) might not display the device
in the scan results if the central device is not on the accept list.

This sample also provides two Bluetooth LE characteristics. To perform a write, devices need
to be bonded, while a read can be done immediately after a connection
(no bonding required).

Requirements
************

* A board with Bluetooth LE support
* Second Bluetooth LE device acting as a central. For example another Zephyr board or smartphone

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_accept_list` in the
Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
