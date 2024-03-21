.. _bluetooth_central_iso:

Bluetooth: Central ISO
######################

Overview
********

Application demonstrating a connected isochronous channel functional as the
central role, by scanning for peripheral devices and establishing a connection
to the first one with a strong enough signal.
The application then attempts to setup a connected isochronous channel and
starts sending data.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_CENTRAL_ISO=y

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/central_iso` in
the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
