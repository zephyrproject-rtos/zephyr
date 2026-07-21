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

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_adv_rsp
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will start PAwR advertising and scan for connectable sync
devices. When a sync device is found, it connects, transfers the sync info via PAST,
assigns a subevent and response slot by writing a GATT characteristic, and then
disconnects. The device will then receive echoed data from each synced device in its
assigned response slot.

Use the :zephyr:code-sample:`ble_periodic_adv_sync_rsp` sample on one or more additional
boards to synchronize and respond to this device.
