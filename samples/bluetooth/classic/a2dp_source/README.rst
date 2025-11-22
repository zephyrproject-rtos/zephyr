.. zephyr:code-sample:: bluetooth_a2dp_source
   :name: a2dp-source
   :relevant-api: bt_a2dp bluetooth

   Use the A2DP APIs.

Overview
********

This sample demonstrates the A2DP (Advanced Audio Distribution Profile) source
functionality using Zephyr's Bluetooth Classic APIs. The application acts as an
A2DP source device, it discovers and connects to A2DP sink devices such as
Bluetooth speaker and headphone, and streams audio data to it automatically.

Requirements
************

* Running on the host with Bluetooth BR/EDR (Classic) support, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

1. Build and flash the sample to the board.
2. The device will discover a2dp sink devices nearby.
3. Connect and stream audio automatically.

This sample can be found under :zephyr_file:`samples/bluetooth/classic/a2dp_source` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
