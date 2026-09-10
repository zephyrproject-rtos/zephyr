.. zephyr:code-sample:: ble_peripheral_ots
   :name: Peripheral Object Transfer Service (OTS)
   :relevant-api: bt_ots bluetooth

   Expose an Object Transfer Service (OTS) GATT Service.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the OTS (Object Transfer) GATT Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_ots
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use the :zephyr:code-sample:`ble_central_otc` sample on a second board to
connect and interact with the Object Transfer Service (OTS), or use a Bluetooth scanner
app with OTS client support (e.g. nRF Connect).
