.. zephyr:code-sample:: uac2-loopback
   :name: USB Audio 2 loopback
   :relevant-api: usbd_api uac2_device

   Implement a USB Audio 2 headset that loops playback audio back to its capture interface.

Overview
********

This sample demonstrates a bidirectional USB Audio Class 2 device using the USB Device support.
It appears to the host as one stereo headset and sends audio received on its playback interface back
on its capture interface.

The sample uses a 48 kHz, 16-bit, stereo format and a clock synchronized to USB Start of Frame.
It therefore does not require an I2S peripheral or an explicit or implicit feedback implementation.
When the capture interface is active without playback, the sample sends silence.

The :zephyr:code-sample:`uac2-explicit-feedback` and :zephyr:code-sample:`uac2-implicit-feedback`
samples demonstrate asynchronous audio clocks connected to hardware audio interfaces.

Requirements
************

A board with USB device support.

Building and Running
********************

This sample can run on any board with USB device support. The following example commands use
:zephyr:board:`nrf52840dongle` as the reference target.

Build and flash the application, then connect the board's USB device port to a host computer:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uac2_loopback
   :board: nrf52840dongle/nrf52840
   :goals: build flash
   :compact:

Testing
*******

Select the Zephyr USB Audio 2 headset as both the playback and capture device. Play audio and record
from the capture interface. The recording should contain the played audio.
