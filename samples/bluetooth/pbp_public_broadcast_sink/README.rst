.. zephyr:code-sample:: bluetooth_public_broadcast_sink
   :name: Public Broadcast Profile (PBP) Public Broadcast Sink
   :relevant-api: bluetooth bt_audio bt_bap bt_pacs bt_pbp

   Use PBP Public Broadcast Sink functionality.

Overview
********

Application demonstrating the PBP Public Broadcast Sink functionality.
Starts by scanning for PBP Public Broadcast Sources and then synchronizes to
the first found source which defines a Public Broadcast Announcement including
a High Quality Public Broadcast Audio Stream configuration.

This sample can be found under
:zephyr_file:`samples/bluetooth/pbp_public_broadcast_sink` in the Zephyr tree.

Check the :zephyr:code-sample-category:`bluetooth` samples for general information.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

When building targeting an nrf52 series board with the Zephyr Bluetooth Controller,
use ``-DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf`` to enable the required ISO
feature support.

Building for an nrf5340dk
-------------------------

You can build both the application core image and an appropriate controller image for the network
core with:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/pbp_public_broadcast_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/pbp_public_broadcast_sink/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:zephyr:code-sample:`bluetooth_hci_ipc` sample
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_iso-bt_ll_sw_split.conf` configuration.

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/pbp_public_broadcast_sink/
   :board: nrf5340bsim/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in :file:`./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/pbp_public_broadcast_sink/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf
