.. zephyr:code-sample:: ble_central_peripheral_hr
   :name: Heart-rate Monitor (Central plus Peripheral)
   :relevant-api: bluetooth

   In Central role, connect to a Bluetooth LE heart-rate monitor and read
   heart-rate measurements.
   In Peripheral role, expose a Heart Rate (HR) GATT Service generating dummy
   heart-rate values.

Overview
********

This sample is a combination of :zephyr:code-sample:`ble_central` and the
:zephyr:code-sample:`ble_peripheral`. The application specifically looks for
heart-rate monitors and reports the heart-rate readings once connected. Also,
exposes the HR (Heart Rate) GATT Service. Once a device connects it will
generate dummy heart-rate values.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/central_peripheral_hr`
in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
