.. zephyr:code-sample:: bluetooth_isochronous_broadcaster
   :name: Isochronous Broadcaster
   :relevant-api: bt_iso bluetooth

   Use the Bluetooth Low Energy Isochronous Broadcaster functionality.

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
the Zephyr tree. Use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable
required ISO feature support in Zephyr Bluetooth Controller on supported boards.
The sample defaults to sequential packing of BIS subevents, add
``-DCONFIG_ISO_PACKING_INTERLEAVED=y`` to use interleaved packing.

Use the sample found under :zephyr_file:`samples/bluetooth/iso_receive` in the
Zephyr tree that will scan, establish a periodic advertising synchronization,
generate BIGInfo reports and synchronize to BIG events from this sample.

See :zephyr:code-sample-category:`bluetooth` samples for details.
