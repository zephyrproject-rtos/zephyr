.. zephyr:code-sample:: bluetooth_ccp_call_control_client
   :name: Call Control Profile (CCP) Call Control Server
   :relevant-api: bluetooth bt_ccp bt_tbs

   CCP Call Control Server sample that registers one or more TBS bearers and advertises the
   TBS UUID(s).

Overview
********

Application demonstrating the CCP Call Control Client functionality.
Starts by scanning for a CCP Call Control Server to connect and set up calls.

The profile works for both GAP Central and GAP Peripheral devices, but this sample only assumes the
GAP Central role.

This sample can be found under :zephyr_file:`samples/bluetooth/ccp_call_control_client`
in the Zephyr tree.

Check the :zephyr:code-sample-category:`bluetooth` samples for general information.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

When building targeting an nrf52 series board with the Zephyr Bluetooth Controller,
use ``-DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf`` to enable the required feature support.

Building for an nrf5340dk
-------------------------

You can build both the application core image and an appropriate controller image for the network
core with:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/ccp_call_control_client/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

If you prefer to only build the application core image, you can do so by doing instead:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/ccp_call_control_client/
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build

In that case you can pair this application core image with the
:zephyr:code-sample:`bluetooth_hci_ipc` sample
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_iso-bt_ll_sw_split.conf` configuration.

Building for a simulated nrf5340bsim
------------------------------------

Similarly to how you would for real HW, you can do:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/ccp_call_control_client/
   :board: nrf5340bsim/nrf5340/cpuapp
   :goals: build
   :west-args: --sysbuild

Note this will produce a Linux executable in :file:`./build/zephyr/zephyr.exe`.
For more information, check :ref:`this board documentation <nrf5340bsim>`.

Building for a simulated nrf52_bsim
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/ccp_call_control_client/
   :board: nrf52_bsim
   :goals: build
   :gen-args: -DOVERLAY_CONFIG=overlay-bt_ll_sw_split.conf
