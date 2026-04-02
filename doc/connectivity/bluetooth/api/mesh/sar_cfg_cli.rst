.. _bluetooth_mesh_sar_cfg_cli:

SAR Configuration Client
########################

The SAR Configuration Client model is a foundation model defined by the Bluetooth Mesh
specification. It is an optional model, enabled with the
:kconfig:option:`CONFIG_BT_MESH_SAR_CFG_CLI` configuration option.

The SAR Configuration Client model is introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and it supports the configuration of the lower transport layer behavior of a node that
supports the :ref:`bluetooth_mesh_sar_cfg_srv` model.

The model can send messages to query or change the states supported by the SAR Configuration Server
(SAR Transmitter and SAR Receiver) using SAR Configuration messages.

The SAR Transmitter procedure is used to determine and configure the SAR Transmitter state of a SAR
Configuration Server. Function calls :c:func:`bt_mesh_sar_cfg_cli_transmitter_get` and
:c:func:`bt_mesh_sar_cfg_cli_transmitter_set` are used to get and set the SAR Transmitter state
of the Target node respectively.

The SAR Receiver procedure is used to determine and configure the SAR Receiver state of a SAR
Configuration Server.  Function calls :c:func:`bt_mesh_sar_cfg_cli_receiver_get` and
:c:func:`bt_mesh_sar_cfg_cli_receiver_set` are used to get and set the SAR Receiver state of the
Target node respectively.

For more information about the two states, see :ref:`bt_mesh_sar_cfg_states`.

An element can send any SAR Configuration Client message at any time to query or change the states
supported by the SAR Configuration Server model of a peer node.  The SAR Configuration Client model
only accepts messages encrypted with the device key of the node supporting the SAR Configuration
Server model.

If present, the SAR Configuration Client model must only be instantiated on the primary element.

API reference
*************

.. doxygengroup:: bt_mesh_sar_cfg_cli
