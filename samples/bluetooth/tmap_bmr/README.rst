.. zephyr:code-sample:: ble_peripheral_tmap_bmr
   :name: Telephone and Media Audio Profile (TMAP) Broadcast Media Receiver (BMR)
   :relevant-api: bluetooth bt_audio bt_bap bt_pacs bt_vcp bt_tmap

   Implement the TMAP Broadcast Media Receiver (BMR) role.

Overview
********

Application demonstrating the TMAP Broadcast Media Receiver functionality.
Implements the BMR role.

Requirements
************

* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/tmap_bmr
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample initializes the TMAP Broadcast Media Receiver (BMR)
role and starts scanning for broadcast audio from a TMAP BMS device.

Use the :zephyr:code-sample:`ble_peripheral_tmap_bms` sample on another board
to act as the Broadcast Media Sender.

See :zephyr:code-sample-category:`bluetooth` samples for details.
