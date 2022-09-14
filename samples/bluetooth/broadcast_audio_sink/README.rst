.. zephyr:code-sample:: bluetooth_broadcast_audio_sink
   :name: Bluetooth: Broadcast Audio Sink
   :relevant-api: bluetooth

   Bluetooth: Broadcast Audio Sink

Overview
********

Application demonstrating the LE Audio broadcast sink functionality.
Starts by scanning for LE Audio broadcast sources and then synchronizes to
the first found and listens to it until the source is (potentially) stopped.

This sample can be found under
:zephyr_file:`samples/bluetooth/broadcast_audio_sink` in the Zephyr tree.

Check the :ref:`bluetooth samples section <bluetooth-samples>` for general information.

Use `CONFIG_TARGET_BROADCAST_NAME` Kconfig to specify the name (CONFIG_BT_DEVICE_NAME)
of a broadcast source to listen to. With default value (empty string), sink
device will listen to all available broadcast sources.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

When building targeting an nrf52 series board with the Zephyr Bluetooth Controller,
use `-DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf` to enable the required ISO
feature support.

Building for an nrf5340dk
-------------------------

You can build both the application core image and an appropriate controller image for the network
core with:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/broadcast_audio_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/broadcast_audio_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:ref:`hci_ipc sample <bluetooth-hci-ipc-sample>`
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_iso-bt_ll_sw_split.conf` configuration.

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/broadcast_audio_sink/
   :board: nrf5340bsim_nrf5340_cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in `./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/broadcast_audio_sink/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf
