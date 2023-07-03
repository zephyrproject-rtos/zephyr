.. _bluetooth_unicast_audio_client:

Bluetooth: Unicast Audio Client
###############################

Overview
********

Application demonstrating the LE Audio unicast client functionality. Scans for and
connects to a LE Audio unicast server and establishes an audio stream.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************
This sample can be found under
:zephyr_file:`samples/bluetooth/unicast_audio_client` in the Zephyr tree.
Use `-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf` to enable required ISO
feature support in Zephyr Bluetooth Controller on supported boards.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
