.. _ble_mesh:

Bluetooth: Mesh
###############

Overview
********

This sample demonstrates Bluetooth Mesh functionality. It has several
standard mesh models, and supports provisioning over both the
Advertising and the GATT Provisioning Bearers (i.e. PB-ADV and PB-GATT).
The application also needs a functioning serial console, since that's
used for the Out-of-Band provisioning procedure.

On boards with LEDs, a Generic OnOff Server model exposes functionality for
controlling the first LED on the board over the mesh.

On boards with buttons, a Generic OnOff Client model will send Onoff messages
to all nodes in the network when the button is pressed.

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

To run the application on an :ref:`nrf5340dk_nrf5340`, a Bluetooth controller application
must also run on the network core. The :ref:`bluetooth-hci-ipc-sample` sample
application may be used. Build this sample with configuration
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_bt_mesh-bt_ll_sw_split.conf`
to enable mesh support.

Interacting with the sample
***************************

The sample can either be provisioned into an existing mesh network with an
external provisioner device, or self-provision through a button press.

When provisioning with a provisioner device, the provisioner must give the
device an Application key and bind it to both Generic OnOff models.

When self-provisioning, the device will take a random unicast address and
bind a dummy Application key to these models.

Once provisioned, messages to the Generic OnOff Server will be used to turn
the LED on or off, and button presses will be used to broadcast OnOff
messages to all nodes in the same network.
