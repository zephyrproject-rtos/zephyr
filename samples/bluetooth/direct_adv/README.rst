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

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/direct_adv
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will start advertising. If no bond exists,
it performs undirected advertising. After pairing completes,
the device reboots after 5 seconds. On subsequent boots it uses
directed advertising towards the bonded peer.
