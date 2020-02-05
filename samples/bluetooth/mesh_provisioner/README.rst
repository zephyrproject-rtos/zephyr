.. _ble_mesh_provisioner:

Bluetooth: Mesh Provisioner
###########################

Overview
********

This sample demonstrates how to use the Bluetooth Mesh APIs related to
provisioning and using the Configuration Database (CDB). It is intended to be
tested together with a device that support the Health Server model. For
example, one could use the sample in :zephyr_file:`tests/bluetooth/mesh_shell`.

The application provisions itself and loads the CDB with an application key.
It then waits to receive an Unprovisioned Beacon from a device which will
trigger provisioning using PB-ADV. Once provisioning is done, the node will
be present in the CDB but not yet marked as configured. The application will
notice the unconfigured node and start configuring it. If no errors are
encountered, the node is marked as configured.

The configuration of a node involves adding an application key, binding the
Health Server model to that application key and then configuring the model to
publish Health Status every 10 seconds.

Please note that this sample uses the CDB API which is currently marked as
EXPERIMENTAL and is likely to change.

Requirements
************

* A board with Bluetooth LE support, or
* QEMU with BlueZ running on the host

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/mesh_provisioner`
in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details on how
to run the sample inside QEMU.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/mesh_provisioner
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.
