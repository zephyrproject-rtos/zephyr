.. _bluetooth_mesh_models_cfg_cli:

Configuration Client
####################

The Configuration Client model is a foundation model defined by the Bluetooth mesh
specification. It provides functionality for configuring most parameters of a
mesh node, including encryption keys, model configuration and feature
enabling.

The Configuration Client model communicates with a
:ref:`bluetooth_mesh_models_cfg_srv` model using the device key of the target
node. The Configuration Client model may communicate with servers on other
nodes or self-configure through the local Configuration Server model.

All configuration functions in the Configuration Client API have ``net_idx``
and ``addr`` as their first parameters. These should be set to the network
index and primary unicast address that the target node was provisioned with.

The Configuration Client model is optional, and it must only be instantiated on the
primary element if present in the Composition Data.

API reference
*************

.. doxygengroup:: bt_mesh_cfg_cli
