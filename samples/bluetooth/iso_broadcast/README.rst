.. _bluetooth-isochronous-broadcaster-sample:

Bluetooth: Isochronous Broadcaster
##################################

Overview
********

A simple application demonstrating the Bluetooth Low Energy Isochronous
Broadcaster functionality.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_ADV_ISO=y

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/iso_broadcast` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/iso_receive` in the
Zephyr tree that will scan, establish a periodic advertising synchronization,
generate BIGInfo reports and synchronize to BIG events from this sample.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
