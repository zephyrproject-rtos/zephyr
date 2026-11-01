.. zephyr:code-sample:: bluetooth_bap_broadcast_source
   :name: Basic Audio Profile (BAP) Broadcast Audio Source
   :relevant-api: bluetooth bt_audio bt_bap

   Use BAP Broadcast Source functionality.

Overview
********

Application demonstrating the BAP Broadcast Source functionality.
Will start advertising extended advertising with audio flags, periodic advertising with the
Broadcast Audio Source Endpoint (BASE) and finally the BIGinfo together with
(mock) Audio (ISO) data.

In addition to the broadcast advertising sets, the sample starts two more
advertising sets that run in parallel with the LE Audio broadcast:

* a connectable **extended** advertising set (device name ``BAP Ext Connectable``)
* a connectable **legacy** advertising set (device name ``BAP Legacy Connectable``)

Remote centrals can connect to either set and reconnect after
disconnecting, while the LE Audio broadcast continues uninterrupted.
The broadcast audio set itself uses ``CONFIG_BT_DEVICE_NAME`` (default
``Broadcast Audio Source``) as its device name in the advertising data.

The BAP Broadcast Source will reset every 30 seconds to show the full API.

Check the :zephyr:code-sample-category:`bluetooth` samples for general information.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

When building targeting an nrf52 series board with the Zephyr Bluetooth Controller,
use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable the required ISO
feature support.
The sample defaults to sequential packing of BIS subevents, add
``-DCONFIG_ISO_PACKING_INTERLEAVED=y`` to use interleaved packing.

Building for an nrf5340dk
-------------------------

You can build both the application core image and an appropriate controller image for the network
core with:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_source/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_source/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:zephyr:code-sample:`bluetooth_hci_ipc` sample
:zephyr_file:`samples/bluetooth/hci_ipc/extra-iso-bt_ll_sw_split.conf` extra configuration.

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_source/
   :board: nrf5340bsim/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in :file:`./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/audio/bap_broadcast_source/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf

USB Audio input
---------------

With :kconfig:option:`CONFIG_USE_USB_AUDIO_INPUT` (enabled for ``nrf54lm20dk/nrf54lm20a/cpuapp``)
the sample is a USB Audio Class 2 speaker that broadcasts the received 48 kHz stereo audio.
The isochronous OUT endpoint is asynchronous and the broadcast is the clock master: the
application measures the input buffer fill level each time an SDU is sent and reports an
explicit feedback value that makes the host follow the Bluetooth clock. If the host does not
follow the feedback, a single sample is dropped or repeated per SDU to keep the buffer bounded,
so that no SDU is sent with missing audio. Statistics are printed every 10 seconds. With an
audio input the broadcast source is not reset.

The USB host can be emulated, for example in BabbleSim, with
``-DEXTRA_CONF_FILE="overlay-bt_ll_sw_split.conf;overlay-emulated-input.conf"``. The emulated
host clock offset is set with :kconfig:option:`CONFIG_AUDIO_INPUT_EMULATED_PPM`, and
:kconfig:option:`CONFIG_AUDIO_INPUT_EMULATED_FEEDBACK` selects whether it follows the feedback.
