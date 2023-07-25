.. _bluetooth_mesh_cfg:

Runtime Configuration
#####################

The runtime configuration API allows applications to change their runtime
configuration directly, without going through the Configuration models.

Bluetooth mesh nodes should generally be configured by a central network
configurator device with a :ref:`bluetooth_mesh_models_cfg_cli` model. Each
mesh node instantiates a :ref:`bluetooth_mesh_models_cfg_srv` model that the
Configuration Client can communicate with to change the node configuration. In some
cases, the mesh node can't rely on the Configuration Client to detect or determine
local constraints, such as low battery power or changes in topology. For these
scenarios, this API can be used to change the configuration locally.

.. note::
   Runtime configuration changes before the node is provisioned will not be stored
   in the :ref:`persistent storage <bluetooth_mesh_persistent_storage>`.

API reference
*************

.. doxygengroup:: bt_mesh_cfg
