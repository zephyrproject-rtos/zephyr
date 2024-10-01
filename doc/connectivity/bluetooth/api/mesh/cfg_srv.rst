.. _bluetooth_mesh_models_cfg_srv:

Configuration Server
####################

The Configuration Server model is a foundation model defined by the Bluetooth Mesh
specification. The Configuration Server model controls most parameters of the
mesh node. It does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_models_cfg_cli` to control it.

The Configuration Server model is mandatory on all Bluetooth Mesh nodes, and
must only be instantiated on the primary element.

API reference
*************

.. doxygengroup:: bt_mesh_cfg_srv
