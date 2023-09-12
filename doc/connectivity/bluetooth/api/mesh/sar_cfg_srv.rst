.. _bluetooth_mesh_sar_cfg_srv:

SAR Configuration Server
########################

The SAR Configuration Server model is a foundation model defined by the Bluetooth mesh
specification. It is an optional model, enabled with the
:kconfig:option:`CONFIG_BT_MESH_SAR_CFG_SRV` configuration option.

The SAR Configuration Server model is introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and it supports the configuration of the
:ref:`segmentation and reassembly (SAR) <bluetooth_mesh_sar_cfg>` behavior of a Bluetooth mesh node.
The model defines a set of states and messages for the SAR configuration.

The SAR Configuration Server model defines two states, SAR Transmitter state and SAR Receiver state.
For more information about the two states, see :ref:`bt_mesh_sar_cfg_states`.

The model also supports the SAR Transmitter and SAR Receiver get and set messages.

The SAR Configuration Server model does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_sar_cfg_cli` to control it.  The SAR Configuration Server model only accepts
messages encrypted with the node’s device key.

If present, the SAR Configuration Server model must only be instantiated on the primary element.

API reference
*************

.. doxygengroup:: bt_mesh_sar_cfg_srv
   :project: Zephyr
   :members:
