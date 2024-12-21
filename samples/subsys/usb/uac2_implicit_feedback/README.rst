.. zephyr:code-sample:: uac2-implicit-feedback
   :name: USB Audio asynchronous implicit feedback sample
   :relevant-api: usbd_api uac2_device i2s_interface

   USB Audio 2 implicit feedback sample playing stereo and recording mono audio
   on I2S interface.

Overview
********

This sample demonstrates how to implement USB asynchronous bidirectional audio
with implicit feedback. The host adjusts number of stereo samples sent for
headphones playback based on the number of mono microphone samples received.

Requirements
************

Target must be able to measure I2S block start (i.e. first sample from output
buffer gets out) relative to USB SOF. The relative offset must be reported with
single sample accuracy.

This sample has been tested on :ref:`nrf5340dk_nrf5340`. While for actual audio
experience it is necessary to connect external I2S ADC and I2S DAC, simple echo
can be accomplished by shorting I2S data output with I2S data input.

Theoretically it should be possible to obtain the timing information based on
I2S and USB interrupts, but currently neither subsystem currently provides
necessary timestamp information.

Building and Running
********************

The code can be found in :zephyr_file:`samples/subsys/usb/uac2_implicit_feedback`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uac2_implicit_feedback
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash
   :compact:
