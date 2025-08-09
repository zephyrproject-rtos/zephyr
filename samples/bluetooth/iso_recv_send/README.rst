.. zephyr:code-sample:: bluetooth_iso_group_chat_core
   :name: Synchronized Receive and Send BISes
   :relevant-api: bt_iso bluetooth

   Core functionality enhancement to expedite Bluetooth LE Group Voice 
   capability.

Overview
********

A simple application demonstrating that shows a LE Audio broadcast sink
device receiving BIS1 and sending on BIS2.

Requirements
************

* Only tested on a Nordic Semiconductor nRF5280 DK, but should also work on
  nRF5340 DK and nRF5340 Audio DK.
* Requires a Bluetooth LE Audio capable controller appropriately enhanced
  and host stack.
* Only tested with the Nordic Semiconductor nRF52840 DK

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/iso_recv_send` in
the Zephyr tree. Use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable
required ISO feature support in Zephyr Bluetooth Controller on supported boards.

Use the sample found under :zephyr_file:`samples/bluetooth/iso_broadcast` on
another board that will start periodic advertising, create BIG to which this
sample will establish periodic advertising synchronization and synchronize to
the Broadcast Isochronous Stream.

See :zephyr:code-sample-category:`bluetooth` samples for details.
