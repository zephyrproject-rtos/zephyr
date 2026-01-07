.. zephyr:code-sample:: bluetooth_a2dp_sink
   :name: A2DP Sink
   :relevant-api: bt_a2dp bluetooth

   Use A2DP (Advanced Audio Distribution Profile) sink functionality.

Overview
********

This sample demonstrates the A2DP (Advanced Audio Distribution Profile) sink
functionality using Zephyr's Bluetooth Classic APIs. The application acts as an
A2DP sink device, it can be discovered and connected from A2DP source devices
such as smartphones, tablets, and computers, and receive and process audio streams
from A2DP source devices.

Requirements
************

* Running on the host with Bluetooth BR/EDR (Classic) support, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

1. Build and flash the sample to the board.
2. The device will become discoverable as "a2dp_sink".
3. Connect and stream audio on your A2DP source device.

This sample can be found under :zephyr_file:`samples/bluetooth/classic/a2dp_sink` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
