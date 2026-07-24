.. zephyr:code-sample:: bluetooth_spp_server
   :name: SPP Server
   :relevant-api: bt_rfcomm bt_sdp bluetooth

   Expose a Bluetooth Classic Serial Port service over RFCOMM.

Overview
********

This sample registers an RFCOMM server, advertises the standard Serial Port
service (UUID 0x1101) in SDP, and echoes received data back to the peer.

Requirements
************

* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/spp_server
   :board: sf32lb52_devkit_lcd/sf32lb525uc6
   :goals: build flash
   :compact:

After flashing, the device becomes discoverable as ``spp_server``. Pair and
connect from a remote RFCOMM client, then open the advertised serial port.

See :zephyr:code-sample-category:`bluetooth_classic` samples for details.
