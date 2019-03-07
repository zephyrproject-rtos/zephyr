.. _ble_mesh:

Bluetooth: Mesh
###############

Overview
********

This sample demonstrates Bluetooth Mesh functionality. It has several
standard Mesh models, and supports provisioning over both the
Advertising and the GATT Provisioning Bearers (i.e. PB-ADV and PB-GATT).
The application also needs a functioning serial console, since that's
used for the Out-of-Band provisioning procedure.

Requirements
************

* A board with Bluetooth LE support, or
* QEMU with BlueZ running on the host

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/mesh` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details on how
to run the sample inside QEMU.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/mesh
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.
