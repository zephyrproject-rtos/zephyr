.. _bluetooth_mesh_od_srv:

On-Demand Private Proxy Server
##############################

The On-Demand Private Proxy Server model is a foundation model defined by the Bluetooth Mesh
specification. It is enabled with the :kconfig:option:`CONFIG_BT_MESH_OD_PRIV_PROXY_SRV` option.

The On-Demand Private Proxy Server model was introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and supports the configuration of advertising with Private Network Identity type of a
node that is a recipient of Solicitation PDUs by managing its On-Demand Private GATT Proxy state.

When enabled, the :ref:`bluetooth_mesh_srpl_srv` is also enabled. The On-Demand Private Proxy Server
is dependent on the :ref:`bluetooth_mesh_models_priv_beacon_srv` to be present on the node.

The On-Demand Private Proxy Server does not have an API of its own, and relies on a
:ref:`bluetooth_mesh_od_cli` to control it. The On-Demand Private Proxy Server model only accepts
messages encrypted with the node's device key.

If present, the On-Demand Private Proxy Server model must only be instantiated on the primary
element.

API reference
*************

.. doxygengroup:: bt_mesh_od_priv_proxy_srv
   :project: Zephyr
   :members:
