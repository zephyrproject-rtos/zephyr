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

See :zephyr:code-sample-category:`bluetooth` samples for details.
