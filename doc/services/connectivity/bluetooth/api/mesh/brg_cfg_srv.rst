.. _bluetooth_mesh_models_brg_cfg_srv:

Bridge Configuration Server
###########################

The Bridge Configuration Server model is a foundation model defined by the Bluetooth Mesh
specification. It is an optional model, enabled with the
:kconfig:option:`CONFIG_BT_MESH_BRG_CFG_SRV` configuration option. This model extends
the :ref:`bluetooth_mesh_models_cfg_srv` model.

The Bridge Configuration Server model was introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and is used for supporting and configuring the Subnet Bridge feature.

The Bridge Configuration Server model relies on a :ref:`bluetooth_mesh_models_brg_cfg_cli` to
configure it. The Bridge Configuration Server model only accepts messages encrypted with the node’s
device key.

If present, the Bridge Configuration Server model must be instantiated on the primary element.

The Bridge Configuration Server model provides access to the following three states:

* Subnet Bridge
* Bridging Table
* Bridging Table Size

For more information about the states, see :ref:`bluetooth_mesh_brg_cfg_states`.

API reference
*************

.. doxygengroup:: bt_mesh_brg_cfg_srv
