.. _bluetooth_broadcast_audio_source:

Bluetooth: Broadcast Audio Source
#################################

Overview
********

Application demonstrating the LE Audio broadcast audio source functionality.
Will start advertising extended advertising with audio flags, periodic advertising with the
broadcast audio source endpoint (BASE) and finally the BIGinfo together with
(mock) Audio (ISO) data.

The broadcast source will reset every 30 seconds to show the full API.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************
This sample can be found under
:zephyr_file:`samples/bluetooth/broadcast_audio_source` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
