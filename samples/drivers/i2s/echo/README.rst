.. _i2s_echo_sample:

I2S Echo Sample
###############

Overview
********

This sample demonstrates how to use an I2S driver in a simple processing of
an audio stream. It configures and starts both the RX and TX streams and then
mixes the original signal with its delayed form that is buffered, providing
a simple echo effect.

Requirements
************

The sample uses the WM8731 audio CODEC that can be found, for example,
on the Audio Codec Shield, but it can be easily adapted to use other
CODECs. The I2S device to be used by the sample is specified by defining
a devicetree node label named ``i2s_rxtx`` or separate node labels ``i2s_rx``
and ``i2s_tx`` if separate I2S devices are to be used for the RX and TX
streams.

This sample has been tested on :ref:`nrf52840dk_nrf52840` (nrf52840dk_nrf52840)
and :ref:`nrf5340dk_nrf5340` (nrf5340dk_nrf5340_cpuapp), using the Audio Codec
Shield, and provides overlay files for both of these boards.

More information about the used shield and the CODEC itself can be found here:

- `Audio Codec Shield`_
- `WM8731`_

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/i2s/echo`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/echo
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Press Button 1 to toggle the echo effect and Button 2 to stop the streams.

.. _Audio Codec Shield: http://wiki.openmusiclabs.com/wiki/AudioCodecShield
.. _WM8731: https://www.cirrus.com/products/wm8731/
