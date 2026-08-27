.. zephyr:code-sample:: ble_central_hr
   :name: Heart-rate Monitor (Central)
   :relevant-api: bluetooth

   Connect to a Bluetooth LE heart-rate monitor and read heart-rate measurements.

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application specifically looks for heart-rate monitors and reports the
heart-rate readings once connected.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_hr
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample scans for peripherals advertising the Heart Rate
Service (HRS). When one is found, it connects and subscribes to Heart Rate
Measurement notifications. Use the :zephyr:code-sample:`ble_peripheral` sample
on a second board, or any Bluetooth LE heart rate monitor, as the peripheral.
