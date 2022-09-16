.. _bluetooth_broadcast_audio_sink:

Bluetooth: Broadcast Audio Sink
###############################

Overview
********

Application demonstrating the LE Audio broadcast sink functionality.
Starts by scanning for LE Audio broadcast sources and then synchronizes to
the first found and listens to it until the source is (potentially) stopped.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************
This sample can be found under
:zephyr_file:`samples/bluetooth/broadcast_audio_sink` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
