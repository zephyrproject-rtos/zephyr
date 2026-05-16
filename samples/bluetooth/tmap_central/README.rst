.. zephyr:code-sample:: ble_peripheral_tmap_central
   :name: Telephone and Media Audio Profile (TMAP) Central
   :relevant-api: bluetooth bt_audio bt_bap bt_cap bt_conn bt_tbs bt_tmap bt_vcp

   Implement the TMAP Call Gateway (CG) and Unicast Media Sender (UMS) roles.

Overview
********

Application demonstrating the TMAP central functionality. Implements the CG and UMS roles.

Requirements
************

* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/tmap_central
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample initializes the TMAP Call Gateway (CG) and Unicast
Media Sender (UMS) roles, then scans for a TMAP peripheral advertising UMR
support. Once connected, it exchanges security and configures unicast audio
streams.

Use the :zephyr:code-sample:`ble_peripheral_tmap_peripheral` sample on another
board to act as the TMAP Peripheral (CT and UMR roles).

See :zephyr:code-sample-category:`bluetooth` samples for details.
