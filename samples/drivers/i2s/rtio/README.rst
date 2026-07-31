.. zephyr:code-sample:: i2s-rtio
   :name: I2S RTIO
   :relevant-api: rtio i2s_interface

   Forward audio blocks from an I2S receiver to an I2S transmitter using the RTIO API.

Overview
********

This sample demonstrates how to use the :ref:`rtio`-based I2S API
(:kconfig:option:`CONFIG_I2S_RTIO`) to continuously forward audio blocks captured by an I2S
receiver to an I2S transmitter, with minimal latency and no extra buffer copies.

Two independent RTIO contexts are used: one for the RX stream, backed by a memory pool
(:c:macro:`RTIO_DEFINE_WITH_MEMPOOL`) so that incoming blocks are allocated on demand, and one for
the TX stream, backed by a plain queue. RX is started with several
:c:func:`rtio_sqe_prep_read_multishot` requests so new blocks keep arriving without having to be
resubmitted one at a time. Every block completed on RX is submitted straight to TX as a write and,
once that write completes, released back to the RX memory pool - no intermediate copy or
processing step is involved.

Each I2S transfer covers 1 ms of 32-bit, 48 kHz stereo audio (384 bytes). The RTIO memory pool
backing RX, however, is carved into smaller 64-byte blocks: :c:macro:`RTIO_DEFINE_WITH_MEMPOOL`
requires a power-of-2 block size, and 384 isn't one, so the sample instead picks the largest power
of 2 that evenly divides 384 and lets a single read request span several contiguous pool blocks (6,
in this case) - filling the full transfer with no leftover or stale bytes, at the cost of a
somewhat higher chance of pool fragmentation.

No external signal source is required to run the sample: with nothing connected to the RX input,
the forwarded stream is simply silence, but the full configure/start/forward/release pipeline is
exercised end-to-end. Boards that expose an ADC and/or DAC codec on their RX/TX I2S controllers can
have them started automatically at boot; see Requirements below.

Requirements
************

This sample requires a board with two I2S (or SAI) controllers, one usable for RX and one for TX,
exposed through the ``i2s-rtio-rx`` and ``i2s-rtio-tx`` devicetree aliases, with an I2S driver that
supports :kconfig:option:`CONFIG_I2S_RTIO`.

Optionally, if audio codecs are wired to the RX and/or TX I2S controllers, they can be started
automatically by also adding ``i2s-rtio-adc`` and/or ``i2s-rtio-dac`` aliases pointing at the
corresponding :ref:`Audio Codec API <audio_codec_api>` devicetree nodes. Without them, the sample
simply loops the raw digital audio between the RX and TX controllers.

This sample has been tested on :zephyr:board:`mimxrt1170_evk` (mimxrt1170_evk@B/mimxrt1176/cm7).

Board support
*************

A board is wired for this sample by adding the ``i2s-rtio-rx`` and ``i2s-rtio-tx`` devicetree
aliases, e.g.:

.. code-block:: devicetree

   / {
           aliases {
                   i2s-rtio-tx = &sai1;
                   i2s-rtio-rx = &sai3;
           };
   };

See :zephyr_file:`samples/drivers/i2s/rtio/boards/mimxrt1170_evk_mimxrt1176_cm7.overlay` for the
complete devicetree and pin configuration used on the :zephyr:board:`mimxrt1170_evk`.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/i2s/rtio`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/rtio
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
   :goals: build flash
   :compact:

No external wiring is required. After flashing, the sample starts the RX stream, waits for the
first blocks to arrive, starts the TX stream, and then continuously forwards every received block
to the transmitter, logging a rate-limited hex dump of each block along the way.
