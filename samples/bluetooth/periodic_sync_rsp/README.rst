.. _bluetooth-periodic-advertising-sync-rsp-sample:

Bluetooth: Periodic Advertising with Responses (PAwR) Synchronization
#####################################################################

Overview
********

A simple application demonstrating the BLE Periodic Advertising with
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

* A board with BLE support
* A controller that supports the Periodic Advertising with Responses (PAwR) - Scanner feature

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/periodic_sync_rsp` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/periodic_adv_rsp` on
another board that will start periodic advertising, which will connect to this
sample and transfer the synchronization info.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
