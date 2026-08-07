.. zephyr:code-sample:: ble_peripheral_gap_svc
   :name: Peripheral GAP service custom implementation
   :relevant-api: bluetooth

   Implement a peripheral with GAP service that limits the accepted names

Overview
********

This sample demonstrates the implementation of the GAP service on the application side.
In this case the GAP name write operation is limited to always start with capital letter.
Once the user connects to the device and tries to set its name, only names starting
with capital letter would be accepted.
Additionally the fact that the name has been changed is logged immediately.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_gap_svc
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect and attempt to
write a new device name via the GAP Device Name characteristic. Names not starting with
a capital letter will be rejected, while accepted names are logged on the device console.
