.. zephyr:code-sample:: ble_periodic_adv_rsp
   :name: Periodic Advertising with Responses (PAwR) Advertiser
   :relevant-api: bt_gap bluetooth

   Use Bluetooth LE Periodic Advertising with Responses (PAwR) Advertiser functionality.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising with
Responses Advertiser functionality.

This sample will scan for the corresponding sync sample and send the required
synchronization info to it. The advertising data is a counter that increases
for each subevent.

Multiple devices can synchronize and respond to one advertiser.

Which subevent to listen to and in which response slot to respond is
application specific. In this sample it is decided by the PAwR advertiser.
Upon connection it will write to a GATT characteristic
the assigned subevent and response slot.

Requirements
************

* A board with Bluetooth LE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Advertiser feature

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_adv_rsp` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_sync_rsp` in the
Zephyr tree that will synchronize and respond to this sample.

See :zephyr:code-sample-category:`bluetooth` samples for details.
