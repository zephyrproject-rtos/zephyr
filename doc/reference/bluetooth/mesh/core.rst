.. _bluetooth_mesh_core:

Core API
########

The Bluetooth Mesh Core API provides functionality for managing the general
Bluetooth Mesh state.

Low Power Node
**************

The Low Power Node (LPN) role allows battery powered devices to participate in
a mesh network as a leaf node. An LPN interacts with the mesh network through
a Friend node, which is responsible for relaying any messages directed to the
LPN. The LPN saves power by keeping its radio turned off, and only wakes up to
either send messages or poll the Friend node for any incoming messages.

The radio control and polling is managed automatically by the mesh stack, but
the LPN API allows the application to trigger the polling at any time through
:c:func:`bt_mesh_lpn_poll`. The LPN operation parameters, including poll
interval, poll event timing and Friend requirements is controlled through the
:option:`CONFIG_BT_MESH_LOW_POWER` option and related configuration options.

API reference
**************

.. doxygengroup:: bt_mesh
   :project: Zephyr
   :members:
