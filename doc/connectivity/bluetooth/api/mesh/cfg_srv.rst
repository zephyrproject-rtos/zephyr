.. _bluetooth_mesh_models_cfg_srv:

Configuration Server
####################

The Configuration Server model is a foundation model defined by the Bluetooth mesh
specification. The Configuration Server model controls most parameters of the
mesh node. It does not have an API of its own, but relies on a
:ref:`bluetooth_mesh_models_cfg_cli` to control it.

.. note::
   The :c:struct:`bt_mesh_cfg_srv` structure has been deprecated. The initial
   values of the Relay, Beacon, Friend, Network transmit and Relay retransmit
   should be set through Kconfig, and the Heartbeat feature should be
   controlled through the :ref:`bluetooth_mesh_heartbeat` API.

The Configuration Server model is mandatory on all Bluetooth mesh nodes, and
must only be instantiated on the primary element.

API reference
*************

.. doxygengroup:: bt_mesh_cfg_srv
