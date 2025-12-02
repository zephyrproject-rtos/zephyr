.. _bluetooth_mesh_models_brg_cfg_cli:

Bridge Configuration Client
###########################

The Bridge Configuration Client is a foundation model defined by the Bluetooth Mesh
specification. The model is optional, and is enabled through
the :kconfig:option:`CONFIG_BT_MESH_BRG_CFG_CLI` option.

The Bridge Configuration Client model provides functionality for configuring the
subnet bridge functionality of another Mesh node containing the
:ref:`bluetooth_mesh_models_brg_cfg_srv`. The device key of the node containing
the target Bridge Configuration Server is used for access layer security.

If present, the Bridge Configuration Client model must only be instantiated on the primary
element.

API reference
*************

.. doxygengroup:: bt_mesh_brg_cfg_cli
