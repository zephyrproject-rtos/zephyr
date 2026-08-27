.. zephyr:code-sample:: ble_peripheral_esp
   :name: ESP Peripheral
   :relevant-api: bt_gatt bt_bas bluetooth

   Expose environmental information using the Environmental Sensing Profile (ESP).

Overview
********
Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the ESP (Environmental Sensing Profile) GATT
Service.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_esp
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device
and subscribe to the temperature sensor characteristics to receive notifications when
the temperature values change. The service also exposes a read-only humidity characteristic.
