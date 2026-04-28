.. _bluetooth_mesh_od_cli:

On-Demand Private Proxy Client
##############################

The On-Demand Private Proxy Client model is a foundation model defined by the Bluetooth Mesh
specification. The model is optional, and is enabled with the
:kconfig:option:`CONFIG_BT_MESH_OD_PRIV_PROXY_CLI` option.

The On-Demand Private Proxy Client model was introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and is used to set and retrieve the On-Demand Private GATT Proxy state. The state
defines how long a node will advertise Mesh Proxy Service with Private Network Identity type after
it receives a Solicitation PDU.

The On-Demand Private Proxy Client model communicates with an On-Demand Private Proxy Server model
using the device key of the node containing the target On-Demand Private Proxy Server model
instance.

If present, the On-Demand Private Proxy Client model must only be instantiated on the primary
element.

Configurations
**************

The On-Demand Private Proxy Client model behavior can be configured with the transmission timeout
option :kconfig:option:`CONFIG_BT_MESH_OD_PRIV_PROXY_CLI_TIMEOUT`.  The
:kconfig:option:`CONFIG_BT_MESH_OD_PRIV_PROXY_CLI_TIMEOUT` controls how long the Client waits for a
state response message to arrive in milliseconds. This value can be changed at runtime using
:c:func:`bt_mesh_od_priv_proxy_cli_timeout_set`.


API reference
*************

.. doxygengroup:: bt_mesh_od_priv_proxy_cli
