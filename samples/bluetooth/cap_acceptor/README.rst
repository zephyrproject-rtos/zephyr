.. zephyr:code-sample:: bluetooth_cap_acceptor
   :name: Bluetooth: Common Audio Profile Acceptor
   :relevant-api: bt_cap bt_bap bluetooth

   CAP Acceptor sample that advertises audio availability to CAP Initiators.

Overview
********

Application demonstrating the CAP Acceptor functionality.
Starts by advertising for a CAP Initiator to connect and set up available streams.

This sample can be found under :zephyr_file:`samples/bluetooth/cap_acceptor` in the Zephyr tree.

Check the :ref:`bluetooth samples section <bluetooth-samples>` for general information.

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
   :zephyr-app: samples/bluetooth/cap_acceptor/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/cap_acceptor/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:ref:`hci_ipc sample <bluetooth-hci-ipc-sample>`
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_iso-bt_ll_sw_split.conf` configuration.

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/cap_acceptor/
   :board: nrf5340bsim/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in `./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/cap_acceptor/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf
