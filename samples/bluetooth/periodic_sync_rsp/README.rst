.. zephyr:code-sample:: ble_periodic_adv_sync_rsp
   :name: Periodic Advertising with Responses (PAwR) Synchronization
   :relevant-api: bt_gap bluetooth

   Implement Bluetooth LE Periodic Advertising with Responses Synchronization.

Overview
********

A simple application demonstrating the Bluetooth LE Periodic Advertising with
Responses Synchronization functionality.

This sample will echo the data received in subevent indications back to the
advertiser.

Which subevent to listen to and in which response slot to respond is
application specific. In this sample it is decided by the PAwR advertiser.
Upon connection it will write to a GATT characteristic
the assigned subevent and response slot.

Flash this sample to multiple devices and they will be given different
subevents and response slots, to that they can communicate with the
advertiser concurrently.

Requirements
************

* A board with Bluetooth LE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Scanner feature

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/periodic_sync_rsp
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will advertise as connectable and wait for the PAwR advertiser
to connect and assign it a subevent and response slot via GATT. Once assigned, it
synchronizes to the PAwR train via PAST and echoes received subevent data back in its
assigned response slot. Multiple boards can be flashed with this sample concurrently,
each receiving a different subevent and response slot assignment.

Use the :zephyr:code-sample:`ble_periodic_adv_rsp` sample on another board to start
PAwR advertising and assign timing to this device.
