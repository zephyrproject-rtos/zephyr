.. zephyr:code-sample:: bluetooth_isochronous_receiver
   :name: Synchronized Receiver
   :relevant-api: bt_iso bluetooth

   Use Bluetooth LE Synchronized Receiver functionality.

Overview
********

A simple application demonstrating the Bluetooth Low Energy Synchronized
Receiver functionality.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_SYNC_ISO=y

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/iso_receive` in
the Zephyr tree. Use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable
required ISO feature support in Zephyr Bluetooth Controller on supported boards.

Use the sample found under :zephyr_file:`samples/bluetooth/iso_broadcast` on
another board that will start periodic advertising, create BIG to which this
sample will establish periodic advertising synchronization and synchronize to
the Broadcast Isochronous Stream.

See :zephyr:code-sample-category:`bluetooth` samples for details.
