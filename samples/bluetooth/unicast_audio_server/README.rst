.. _bluetooth_unicast_audio_server:

Bluetooth: Unicast Audio Server
###############################

Overview
********

Application demonstrating the LE Audio unicast server functionality.
Starts advertising and awaits connection from a LE Audio unicast client.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************
This sample can be found under
:zephyr_file:`samples/bluetooth/unicast_audio_server` in the Zephyr tree.
Use `-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf` to enable required ISO
feature support in Zephyr Bluetooth Controller on supported boards.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
