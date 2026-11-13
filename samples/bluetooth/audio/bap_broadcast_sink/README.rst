.. zephyr:code-sample:: bluetooth_bap_broadcast_sink
   :name: Basic Audio Profile (BAP) Broadcast Audio Sink
   :relevant-api: bluetooth bt_audio bt_bap bt_conn bt_pacs

   Use BAP Broadcast Sink functionality.

Overview
********

Application demonstrating the BAP Broadcast Sink functionality.
Starts by scanning for BAP Broadcast Sources and then synchronizes to
the first found and listens to it until the source is (potentially) stopped.

Check the :zephyr:code-sample-category:`bluetooth` samples for general information.

Use :kconfig:option:`CONFIG_TARGET_BROADCAST_NAME` Kconfig to specify the name
(:kconfig:option:`CONFIG_BT_DEVICE_NAME`) of a broadcast source to listen to. With default value
(empty string), sink device will listen to all available broadcast sources.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

When building targeting an nrf52 series board with the Zephyr Bluetooth Controller,
use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable the required ISO
feature support.

Building for an nrf5340dk
-------------------------

You can build both the application core image and an appropriate controller image for the network
core with:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:zephyr:code-sample:`bluetooth_hci_ipc` sample
:zephyr_file:`samples/bluetooth/hci_ipc/extra-iso-bt_ll_sw_split.conf` extra configuration.

Building for an nRF5340 Audio DK
--------------------------------

On the :zephyr:board:`nrf5340_audio_dk` the sample uses USB Audio as output by default. With
``FILE_SUFFIX=i2s_codec`` it plays the received audio on the headphone output through the on-board
CS47L63 codec instead (:kconfig:option:`CONFIG_USE_I2S_CODEC_AUDIO_OUTPUT`). Both channels of a
stereo broadcast are mixed to the mono headphone output. Buttons 1 and 2 lower and raise the
volume, button 3 mutes and unmutes.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_sink/
   :board: nrf5340_audio_dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DFILE_SUFFIX=i2s_codec

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_sink/
   :board: nrf5340bsim/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in :file:`./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_sink/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf
