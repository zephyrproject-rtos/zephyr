.. zephyr:code-sample:: ble_peripheral
   :name: Peripheral
   :relevant-api: bt_gatt bluetooth

   Implement basic Bluetooth LE Peripheral role functionality (advertising and exposing GATT services).

Overview
********

Application demonstrating the Bluetooth LE Peripheral role. It has several well-known and
vendor-specific GATT services that it exposes.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect)
or the :zephyr:code-sample:`ble_central` sample on a second board,
or any Bluetooth LE device to connect to the device.
The sample exposes the following GATT services: Battery (BAS), Current Time (CTS),
Heart Rate (HRS), Immediate Alert (IAS), and a vendor-specific service.
