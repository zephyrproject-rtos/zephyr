.. _bt_mesh_proxy:

Proxy
#####

The Proxy feature allows legacy devices like phones to access the Bluetooth
mesh network through GATT. The Proxy feature is only compiled in if the
:kconfig:option:`CONFIG_BT_MESH_GATT_PROXY` option is set. The Proxy feature state is
controlled by the :ref:`bluetooth_mesh_models_cfg_srv`, and the initial value
can be set with :c:member:`bt_mesh_cfg_srv.gatt_proxy`.

API reference
*************

.. doxygengroup:: bt_mesh_proxy
