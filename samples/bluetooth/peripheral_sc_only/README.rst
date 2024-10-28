.. zephyr:code-sample:: ble_peripheral_sc_only
   :name: Peripheral SC-only
   :relevant-api: bt_conn bluetooth

   Enable "Secure Connections Only" mode for a Bluetooth LE peripheral.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application enables the Secure Connections Only mode, i.e. will only
accept connections that are secured using security level 4 (FIPS).


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_sc_only`
in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
