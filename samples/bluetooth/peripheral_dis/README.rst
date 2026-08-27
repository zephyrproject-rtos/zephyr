.. zephyr:code-sample:: ble_peripheral_dis
   :name: DIS Peripheral
   :relevant-api: bt_gatt bluetooth

   Expose device information using the Device Information Service (DIS).

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the DIS (Device Information) GATT Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_dis
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device
and read the Device Information Service (DIS) characteristics such as manufacturer name,
model number, and firmware revision.
