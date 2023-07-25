.. _peripheral_hids:

Bluetooth: Peripheral HIDs
##########################

Overview
********

Similar to the :ref:`Peripheral <ble_peripheral>` sample, except that this
application specifically exposes the HID GATT Service. The report map used is
for a generic mouse.

In the default configuration the sample uses passkey authentication (displays a
code on the peripheral and requires that to be entered on the host during
pairing) and requires an authenticated link to access the GATT characteristics.
To disable authentication and just use encrypted channels instead, build the
sample with `CONFIG_SAMPLE_BT_USE_AUTHENTICATION=n`.

Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_hids` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
