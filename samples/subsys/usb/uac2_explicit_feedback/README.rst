.. zephyr:code-sample:: uac2-explicit-feedback
   :name: USB Audio asynchronous explicit feedback sample
   :relevant-api: usbd_api uac2_device i2s_interface

   USB Audio 2 explicit feedback sample playing audio on I2S.

Overview
********

This sample demonstrates how to implement USB asynchronous audio playback with
explicit feedback. It can run on any board with USB and I2S support, but the
feedback calculation is currently only implemented for the Nordic nRF5340 IC.

The device running this sample presents itself to the host as a Full-Speed
Asynchronous USB Audio 2 class device supporting 48 kHz 16-bit 2-channel
(stereo) playback.

.. warning::
   Microsoft Windows USB Audio 2.0 driver available since Windows 10,
   release 1703 expects Full-Speed explicit feedback endpoint wMaxPacketSize to
   be equal 4, which violates the USB 2.0 Specification.
   See https://aka.ms/AArvnax for Windows Feedback Hub report.

Explicit Feedback
*****************

Asynchronous USB Audio is used when the actual sample clock is not controlled by
USB host. Because the sample clock is independent from USB SOF it is inevitable
that 1 ms according to audio sink (I2S) will be either slightly longer or
shorter than 1 ms according to audio source (USB host). In the long run, this
discrepancy leads to overruns or underruns. By providing explicit feedback to
host, the device can tell host how many samples on average it needs every frame.
The host achieves the average by sending either nominal or nominal ±1 sample
packets every frame.

The dummy feedback implementation, used when there is no target-specific
feedback code available, returns a feedback value that results in host sending
nominal number of samples every frame. Theoretically it should be possible to
obtain the timing information based on I2S and USB interrupts, but currently
neither subsystem provides the necessary timestamp information.

Explcit Feedback on nRF5340
***************************

The nRF5340 is capable of counting both edges of I2S LRCLK relative to USB SOF
with the use of DPPI, TIMER and GPIOTE input. Alternatively, if the GPIOTE input
is not available, the DPPI and TIMER peripherals on nRF5340 can be configured to
provide relative timing information between I2S FRAMESTART and USB SOF.

This sample in both modes (direct sample counting and indirect I2S buffer output
to USB SOF offset) has been tested on :ref:`nrf5340dk_nrf5340`.

The sample defaults to indirect feedback calculation because direct sample
counting requires external connection between I2S LRCLK output pin to GPIOTE
input pin (hardcoded to P1.09) on :ref:`nrf5340dk_nrf5340`. In the indirect mode
no extra connections are necessary and the sample can even be used without any
I2S device connected where I2S signals can be checked e.g. on logic analyzer.

Building and Running
********************

The code can be found in :zephyr_file:`samples/subsys/usb/uac2_explicit_feedback`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uac2_explicit_feedback
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash
   :compact:
