.. _peripheral_iso:

Bluetooth: Peripheral ISO
#########################

Overview
********

Similar to the :ref:`Peripheral <ble_peripheral>` sample, except that this application enables
support for connected isochronous (ISO) channels.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_PERIPHERAL_ISO=y

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_iso` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
