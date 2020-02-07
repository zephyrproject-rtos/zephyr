.. _bluetooth_central_hr:

Bluetooth: Central / Heart-rate Monitor
#######################################

Overview
********

Similar to the :ref:`Central <bluetooth_central>` sample, except that this
application specifically looks for heart-rate monitors and reports the
heart-rate readings once connected.
If TRACK_VND=1 is passed to cmake build system, the application looks for
devices advertising a Vendor specific UUID128 and pair with them, if required
by the permissions, reading the value.

Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/central_hr` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
