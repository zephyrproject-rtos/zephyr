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

See :zephyr:code-sample-category:`bluetooth` samples for details.
