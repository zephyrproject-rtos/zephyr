.. zephyr:code-sample:: ble_mesh_provisioner
   :name: Mesh Provisioner
   :relevant-api: bt_mesh bluetooth

   Provision a node and configure it using the Bluetooth Mesh APIs.

Overview
********

This sample demonstrates how to use the Bluetooth Mesh APIs related to
provisioning and using the Configuration Database (CDB). It is intended
to be tested together with a device capable of being provisioned. For
example, one could use the sample in
:zephyr_file:`samples/bluetooth/mesh`
or :zephyr_file:`tests/bluetooth/mesh_shell`.

The application provisions itself and loads the CDB with an application
key, then waits to receive an Unprovisioned Beacon from a device. If the
board has a push button connected via GPIO and configured using the
``sw0`` :ref:`devicetree <dt-guide>` alias, the application then waits
for the user to press the button, which will trigger provisioning using
PB-ADV. If the board doesn't have the push button, the sample will
provision detected devices automatically. Once provisioning is done, the
node will be present in the CDB but not yet marked as configured. The
application will notice the unconfigured node and start configuring it.
If no errors are encountered, the node is marked as configured.

The configuration of a node involves adding an application key, getting
the composition data, and binding all its models to the application key.

Requirements
************

* A board with Bluetooth LE support, or
* QEMU with BlueZ running on the host

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/mesh_provisioner` in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details on
how to run the sample inside QEMU.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/mesh_provisioner
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

To run the application on an :zephyr:board:`nrf5340dk`, a Bluetooth controller application
must also run on the network core. The :zephyr:code-sample:`bluetooth_hci_ipc` sample
application may be used. Build this sample with configuration
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_bt_mesh-bt_ll_sw_split.conf`
to enable mesh support.
