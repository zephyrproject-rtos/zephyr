.. _bluetooth-iso-receive-sample:

Bluetooth: Synchronized Receiver
###############################################

Overview
********

A simple application demonstrating the Bluetooth Low Energy Synchronized
Receiver functionality.

Requirements
************

* A board with Bluetooth Low Energy support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/iso_receive` in
the Zephyr tree.

Use the sample found under :zephyr_file:`samples/bluetooth/iso_broadcast` on
another board that will start periodic advertising, create BIG to which this
sample will establish periodic advertising synchronization and synchronize to
the Broadcast Isochronous Stream.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
