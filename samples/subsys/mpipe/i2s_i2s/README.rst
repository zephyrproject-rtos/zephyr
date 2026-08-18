.. zephyr:code-sample:: i2s-i2s
   :name: i2s-i2s pipeline
   :relevant-api: mpipe_aud mpipe_base

   Capture audio from an I2S receiver, apply gain, and play it back through an
   I2S codec.

Overview
********

This sample builds an I2S-to-I2S audio loop on top of the Multimedia Pipeline
framework: frames captured from an I2S receiver pass through a gain stage and are
rendered by an I2S codec sink.

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     i2s_src   [label="I2S\nSource"];
     caps      [label="Caps\nFilter"];
     gain      [label="Gain\nTransform"];
     i2s_codec [label="I2S Codec\nSink"];
     i2s_src -> caps -> gain -> i2s_codec;
   }

- **I2S source** - captures PCM frames from an I2S receiver.
- **Caps filter** - pins the frame interval and channel count so the negotiated
  format is concrete.
- **Gain transform** - scales the sample values.
- **I2S codec sink** - renders the frames through an I2S codec.

The sink's codec is linked from its I2S node with a ``codec`` phandle, so the
application configures only the I2S device and the codec follows from
devicetree. Because that makes the codec a device dependency of the I2S
controller, it must initialize first (see ``CONFIG_AUDIO_CODEC_INIT_PRIORITY``
in :file:`prj.conf`).

Requirements
************

* A board with two I2S interfaces, or one supporting simultaneous RX and TX
* An I2S codec reachable from the transmit interface

Building and Running
********************

On ``native_sim`` the I2S interfaces are backed by files. Provide a raw PCM
capture file for the receiver and a path for the transmitted output:

.. code-block:: console

   west build -b native_sim/native/64 samples/subsys/mpipe/i2s_i2s
   ./build/zephyr/zephyr.exe --i2s_rx_rx=input.pcm --i2s_tx_tx=output.pcm

The exact per-instance option names are printed by ``zephyr.exe --help``. With
no receive file the pipeline still starts and streams silence.
