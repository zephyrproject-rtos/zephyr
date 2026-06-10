.. zephyr:code-sample:: bluetooth_a2dp_source
   :name: A2DP Source
   :relevant-api: bt_a2dp bluetooth

   Use A2DP (Advanced Audio Distribution Profile) source functionality.

Overview
********

This sample demonstrates the A2DP (Advanced Audio Distribution Profile) source
functionality using Zephyr's Bluetooth Classic APIs. The application acts as an
A2DP source device, it discovers and connects to a A2DP sink device such as
Bluetooth speaker and headphone, and streams audio data to it automatically.

Requirements
************

* Running on the host with Bluetooth BR/EDR (Classic) support, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/a2dp_source
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
   :goals: build flash
   :compact:

After flashing, the device discovers nearby A2DP sink devices and starts streaming audio
automatically.

See :zephyr:code-sample-category:`bluetooth` samples for details.
