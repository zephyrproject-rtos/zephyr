.. zephyr:code-sample:: ble_cs
   :name: Channel Sounding
   :relevant-api: bt_gap bluetooth

   Use Channel Sounding functionality.


Bluetooth: Channel Sounding
###########################

Overview
********

These samples demonstrates how to use the Bluetooth Channel Sounding feature.

The CS Test sample shows how to us the CS test command to override randomization of certain channel
sounding parameters, experiment with different configurations, or evaluate the RF medium. It can
be found under :zephyr_file:`samples/bluetooth/channel_sounding/cs_test`.

The connected CS sample shows how to set up regular channel sounding procedures on a connection
between two devices.
It can be found under :zephyr_file:`samples/bluetooth/channel_sounding/connected_cs`

A basic distance estimation algorithm is included in both.

Requirements
************

* A board with BLE support
* A controller that supports the Channel Sounding feature

Building and Running
********************

These samples can be found under :zephyr_file:`samples/bluetooth/channel_sounding` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
