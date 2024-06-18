.. zephyr:code-sample:: usb-audio-headset
   :name: USB Audio headset
   :relevant-api: _usb_device_core_api

   Implement a USB Audio headset device with audio IN/OUT loopback.

Overview
********

This sample app demonstrates use of a USB Audio driver by the Zephyr
project. This very simple sample that performs loopback over IN/OUT
ISO endpoints. The device will show up as single audio device.

Building and Running
********************

In order to build the sample an overlay file with required options
must be provided. By default app.overlay is added. An overlay contains
software and hardware specific information which allow to fully
describe the device.

After you have built and flashed the sample app image to your board, plug the
board into a host device.

Testing
*******

Steps to test the sample:

- Build and flash the sample as described above.
- Connect to the HOST.
- Chose default Audio IN/OUT.
- Start streaming audio (for example by playing an audio file on the HOST).
- Start recording audio stream (for example using Audacity).
- Verify the recorded audio stream.

This sample can be found under
:zephyr_file:`samples/subsys/usb/audio/headset` in the Zephyr project tree.
