.. _bluetooth_mesh_models_priv_beacon_cli:

Private Beacon Client
#####################

The Private Beacon Client model is a foundation model defined by the Bluetooth
mesh specification. It is enabled with the
:kconfig:option:`CONFIG_BT_MESH_PRIV_BEACON_CLI` option.

The Private Beacon Client model is introduced in the Bluetooth Mesh Profile
Specification version 1.1, and provides functionality for configuring the
:ref:`bluetooth_mesh_models_priv_beacon_srv` models.

The Private Beacons feature adds privacy to the different Bluetooth mesh
beacons by periodically randomizing the beacon input data. This protects the
mesh node from being tracked by devices outside the mesh network, and hides the
network's IV index, IV update and the Key Refresh state.

The Private Beacon Client model communicates with a
:ref:`bluetooth_mesh_models_priv_beacon_srv` model using the device key of the
target node. The Private Beacon Client model may communicate with servers on
other nodes or self-configure through the local Private Beacon Server model.

All configuration functions in the Private Beacon Client API have ``net_idx``
and ``addr`` as their first parameters. These should be set to the network
index and the primary unicast address the target node was provisioned with.

The Private Beacon Client model is optional, and can be instantiated on any
element.

API reference
*************

.. doxygengroup:: bt_mesh_priv_beacon_cli
   :project: Zephyr
   :members:
