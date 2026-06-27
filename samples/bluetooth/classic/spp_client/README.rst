.. zephyr:code-sample:: bluetooth_spp_client
   :name: SPP Client
   :relevant-api: bt_rfcomm bt_sdp bluetooth

   Connect to a remote Bluetooth Classic Serial Port service over RFCOMM.

Overview
********

This sample scans for BR/EDR devices, connects to the first discovered peer,
discovers the standard Serial Port service (UUID 0x1101) via SDP, opens an
RFCOMM channel, and sends periodic test data.

Requirements
************

* A board with Bluetooth BR/EDR (Classic) support
* A remote device running the :zephyr:code-sample:`bluetooth_spp_server` sample or
  any other device exposing a Serial Port RFCOMM service

Building and Running
********************

Build and flash the SPP server sample on a second board, then build and flash
this client:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/spp_client
   :board: sf32lb52_devkit_lcd/sf32lb525uc6
   :goals: build flash
   :compact:

See :zephyr:code-sample-category:`bluetooth_classic` samples for details.
