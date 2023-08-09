.. _bluetooth_mesh_models_rpr_srv:

Remote Provisioning Server
##########################

The Remote Provisioning Server model is a foundation model defined by the Bluetooth
mesh specification. It is enabled with the
:kconfig:option:`CONFIG_BT_MESH_RPR_SRV` option.

The Remote Provisioning Server model is introduced in the Bluetooth Mesh Protocol
Specification version 1.1, and is used to support the functionality of remotely
provisioning devices into a mesh network.

The Remote Provisioning Server does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_models_rpr_cli` to control it. The Remote Provisioning Server
model only accepts messages encrypted with the node's device key.

If present, the Remote Provisioning Server model must be instantiated on the primary
element.

Note that after refreshing the device key, node address or Composition Data through a Node Provisioning Protocol
Interface (NPPI) procedure, the :c:member:`bt_mesh_prov.reprovisioned` callback is triggered. See section
:ref:`bluetooth_mesh_models_rpr_cli` for further details.

API reference
*************

.. doxygengroup:: bt_mesh_rpr_srv
   :project: Zephyr
   :members:
