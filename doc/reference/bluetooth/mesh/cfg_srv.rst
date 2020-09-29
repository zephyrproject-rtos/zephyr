.. _bluetooth_mesh_models_cfg_srv:

Configuration Server
####################

Configuration Server model is a foundation model defined by the Bluetooth Mesh
specification. The Configuration Server model controls most parameters of the
mesh node. It does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_models_cfg_cli` to control it.

The application can configure the initial parameters of the Configuration
Server model through the :c:struct:`bt_mesh_cfg_srv` instance passed to
:c:macro:`BT_MESH_MODEL_CFG_SRV`. Note that if the mesh node stored changes to
this configuration in the settings subsystem, the initial values may be
overwritten upon loading.

The Configuration Server model is mandatory on all Bluetooth Mesh nodes, and
should be instantiated in the first element.

API reference
*************

.. doxygengroup:: bt_mesh_cfg_srv
   :project: Zephyr
   :members:
