.. _bluetooth_mesh_models_priv_beacon_srv:

Private Beacon Server
#####################

The Private Beacon Server model is a foundation model defined by the Bluetooth
mesh specification. It is enabled with
:kconfig:option:`CONFIG_BT_MESH_PRIV_BEACON_SRV` option.

The Private Beacon Server model is introduced in the Bluetooth Mesh Protocol
Specification version 1.1, and controls the mesh node's Private Beacon state,
Private GATT Proxy state and Private Node Identity state.

The Private Beacons feature adds privacy to the different Bluetooth Mesh
beacons by periodically randomizing the beacon input data. This protects the
mesh node from being tracked by devices outside the mesh network, and hides the
network's IV index, IV update and the Key Refresh state. The Private Beacon Server
must be instantiated for the device to support sending of the private beacons,
but the node will process received private beacons without it.

The Private Beacon Server does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_models_priv_beacon_cli` to control it. The Private Beacon
Server model only accepts messages encrypted with the node's device key.

The application can configure the initial parameters of the Private Beacon
Server model through the :c:struct:`bt_mesh_priv_beacon_srv` instance passed to
:c:macro:`BT_MESH_MODEL_PRIV_BEACON_SRV`. Note that if the mesh node stored
changes to this configuration in the settings subsystem, the initial values may
be overwritten upon loading.

If present, the Private Beacon Server model must only be instantiated on the primary element.

API reference
*************

.. doxygengroup:: bt_mesh_priv_beacon_srv
   :project: Zephyr
   :members:
