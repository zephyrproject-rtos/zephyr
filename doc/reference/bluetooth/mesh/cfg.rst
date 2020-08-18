.. _bluetooth_mesh_cfg:

Runtime configuration
#####################

The Bluetooth Mesh runtime configuration API allows applications to change
their runtime configuration directly, without going through the Configuration
models.

Bluetooth Mesh nodes should generally be configured by a central network
configurator device with a :ref:`bluetooth_mesh_models_cfg_cli` model. Each
mesh node instantiates a :ref:`bluetooth_mesh_models_cfg_srv` model that the
Config Client can communicate with to change the node configuration. In some
cases, the mesh node can't rely on the Config Client to detect or determine
local constraints, such as low battery power or changes in topology. For these
scenarios, this API can be used to change the configuration locally.

API reference
*************

.. doxygengroup:: bt_mesh_cfg
   :project: Zephyr
   :members:
