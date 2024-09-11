.. zephyr:code-sample:: ble_direct_adv
   :name: Direct Advertising
   :relevant-api: bluetooth

   Advertise directly to a bonded central device.

Overview
********

Application demonstrating the Bluetooth LE Direct Advertising capability. If no device is bonded
to the peripheral, casual advertising will be performed. Once bonded, on every subsequent
boot direct advertising to the bonded central will be performed. Additionally this sample
provides two Bluetooth LE characteristics. To perform write, devices need to be bonded, while read
can be done just after connection (no bonding required).

Please note that direct advertising towards iOS based devices is not allowed.
For more information about designing Bluetooth LE devices for Apple products refer to
"Accessory Design Guidelines for Apple Devices".

Requirements
************

* A board with Bluetooth LE support
* Second Bluetooth LE device acting as a central with enabled privacy. For example another Zephyr board
  or any modern smartphone

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/direct_adv` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
