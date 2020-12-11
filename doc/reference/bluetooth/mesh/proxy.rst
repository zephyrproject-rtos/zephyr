.. _bt_mesh_proxy:

Proxy
#####

The Bluetooth Mesh Proxy feature allows legacy devices like phones to access
the Mesh network through GATT. The Proxy feature is only compiled in if the
:option:`CONFIG_BT_MESH_GATT_PROXY` option is set. The Proxy feature state is
controlled by the :ref:`bluetooth_mesh_models_cfg_srv`, and the initial value
can be set with :c:member:`bt_mesh_cfg_srv.gatt_proxy`.

API reference
*************

.. doxygengroup:: bt_mesh_proxy
   :project: Zephyr
   :members:
