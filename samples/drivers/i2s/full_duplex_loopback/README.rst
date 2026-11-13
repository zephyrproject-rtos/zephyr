.. zephyr:code-sample:: i2s-full-duplex-loopback
   :name: I2S full duplex loopback
   :relevant-api: i2s_interface

   Check that the RX and TX streams of one I2S device run independently.

Overview
********

This sample configures both directions of an I2S device with one call to
:c:func:`i2s_configure` using ``I2S_DIR_BOTH``, and streams data out of the
transmitter into the receiver of the same device. It then checks that:

* the data comes back intact and with the left and right channels in place,
* stopping the transmitter does not stop the receiver, and the transmitter can
  be started again while the receiver keeps running,
* stopping the receiver does not stop the transmitter, and the receiver can be
  started again while the transmitter keeps running,
* no memory block is lost.

The checks run once with a continuous bit clock and once with a gated one.

Requirements
************

The I2S device to use is selected by giving it the devicetree node label
``i2s_rxtx``. The driver has to run both directions of the device at the same
time. For the STM32 I2S driver, the node has to be one that supports full
duplex (``st,full-duplex``) and both the data input and the data output have to
be listed in ``pinctrl-0``.

The data output has to be connected to the data input with a wire.

Building and Running
********************

The sample has been written for :zephyr:board:`nucleo_h745zi_q`
(nucleo_h745zi_q/stm32h745xx/m7). Connect ``PC12`` (``I2S3_SDO``) to ``PC11``
(``I2S3_SDI``). The clock and word select pins are only used internally in this
loopback and can stay unconnected.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/full_duplex_loopback
   :board: nucleo_h745zi_q/stm32h745xx/m7
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   I2S full duplex loopback sample
   T1 loopback, bit clock continuous
     loopback: 1280 frames, 0 breaks, 0 misaligned
   T2 stop TX while RX runs, bit clock continuous
     before the stop: 1280 frames, 0 breaks, 0 misaligned
     after the restart: 1280 frames, 0 breaks, 0 misaligned
   T3 stop RX while TX runs, bit clock continuous
     before the stop: 1280 frames, 0 breaks, 0 misaligned
     after the restart: 1280 frames, 0 breaks, 0 misaligned
   ...
   Full duplex loopback: PASS

If every frame is reported as misaligned, or the data is shifted by one word,
the receiver locked onto the wrong channel when it started late. If the
received data is all zero, the data input might need the swap that the
``ioswp`` property provides.
