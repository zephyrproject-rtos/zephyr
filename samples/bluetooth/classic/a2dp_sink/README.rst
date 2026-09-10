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

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/a2dp_sink
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
   :goals: build flash
   :compact:

After flashing, the device becomes discoverable as ``a2dp_sink``. Connect and stream audio
from your A2DP source device.

See :zephyr:code-sample-category:`bluetooth` samples for details.
