.. zephyr:code-sample:: ble_peripheral_tmap_bms
   :name: Telephone and Media Audio Profile (TMAP) Broadcast Media Sender (BMS)
   :relevant-api: bluetooth bt_audio bt_bap bt_cap bt_tmap

   Implement the LE Audio TMAP Broadcast Media Sender (BMS) role.

Overview
********

Application demonstrating the TMAP Broadcast Media Sender functionality.
Implements the BMS role.

Requirements
************

* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/tmap_bms
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample initializes the TMAP Broadcast Media Sender (BMS)
role and starts broadcasting an audio stream.

Use the :zephyr:code-sample:`ble_peripheral_tmap_bmr` sample on another board
to receive the broadcast audio stream.

See :zephyr:code-sample-category:`bluetooth` samples for details.
