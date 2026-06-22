.. zephyr:code-sample:: ble_peripheral_csc
   :name: Cycling Speed and Cadence (CSC) Peripheral
   :relevant-api: bt_gatt bluetooth

   Expose a Cycling Speed and Cadence (CSC) GATT Service.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the CSC (Cycling Speed and Cadence) GATT
Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_csc
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device
and subscribe to the CSC Measurement characteristic to receive cycling speed and cadence
notifications.
