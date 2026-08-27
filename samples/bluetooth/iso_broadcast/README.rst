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

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/iso_broadcast
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample starts Isochronous Broadcasting automatically.

Use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable
required ISO feature support in Zephyr Bluetooth Controller on supported boards.
The sample defaults to sequential packing of BIS subevents, add
``-DCONFIG_ISO_PACKING_INTERLEAVED=y`` to use interleaved packing.

Use the :zephyr:code-sample:`bluetooth_isochronous_receiver` sample on another
board, which will scan, establish a periodic advertising synchronization,
generate BIGInfo reports and synchronize to BIG events from this sample.
