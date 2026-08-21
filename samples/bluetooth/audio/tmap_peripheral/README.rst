.. zephyr:code-sample:: ble_peripheral_tmap_peripheral
   :name: Telephone and Media Audio Profile (TMAP) Peripheral
   :relevant-api: bluetooth bt_audio bt_bap bt_csip bt_mcc bt_tbs bt_tmap bt_vcp

   Implement the TMAP Call Terminal (CT) and Unicast Media Receiver (UMR) roles.

Overview
********

Application demonstrating the TMAP peripheral functionality. Implements the CT and UMR roles.

Requirements
************

* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/tmap_peripheral
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample initializes the TMAP Call Terminal (CT) and Unicast
Media Receiver (UMR) roles, then starts advertising as ``TMAP Peripheral``.
After a TMAP Central connects and security is established, the sample discovers
the peer's TMAP role: if the peer is a Call Gateway (CG), it sends an "originate call" request
and terminates it after 2 seconds; if the peer is a Unicast Media Sender (UMS),
it sends a "play media" request and pauses after 2 seconds.

Use the :zephyr:code-sample:`ble_peripheral_tmap_central` sample on another
board to act as the TMAP Central (CG and UMS roles).

Playback to an audio codec
**************************

The sample can decode LC3 audio and play it out to a hardware codec over I2S.
Playback is optional: when the required devicetree nodes or Kconfig symbols
are absent, ``le_audio_playback.c`` compiles to a stub and the sample still
runs as a UMR that acknowledges the stream but discards the audio.

To enable playback on a new board, provide the following in a board overlay
(or board :file:`.dts`):

* an ``i2s-codec-tx`` DT alias pointing at the I2S peripheral wired to the codec
* a devicetree node labelled ``audio_codec`` implementing the Zephyr
  :ref:`audio_codec_api`

and add a matching board :file:`.conf` under :file:`boards/` enabling:

.. code-block:: cfg

   CONFIG_LIBLC3=y
   CONFIG_I2S=y
   CONFIG_AUDIO=y
   CONFIG_AUDIO_CODEC=y

For interoperability with commercial smartphone LE Audio sources, the sample
also sets ``TMAP_PERIPHERAL_STEREO=y`` (advertise stereo location) and
``TMAP_PERIPHERAL_AUTO_CTRL=n`` (behave as a passive sink). Refer to the
existing entries under :file:`boards/` for concrete examples.

See :zephyr:code-sample-category:`bluetooth` samples for details.
