.. zephyr:code-sample:: ble_central_ht
   :name: Health Thermometer (Central)
   :relevant-api: bluetooth

   Connect to a Bluetooth LE health thermometer sensor and read temperature measurements.

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application specifically looks for health thermometer sensor and reports the
die temperature readings once connected.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_ht
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample scans for peripherals advertising the Health
Thermometer Service (HTS). When one is found, it connects and subscribes
to Temperature Measurement indications. Use the
:zephyr:code-sample:`ble_peripheral_ht` sample on a second board as the
peripheral.
