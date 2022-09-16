.. _bluetooth_mesh_core:

Core
####

The core provides functionality for managing the general Bluetooth mesh
state.

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
:kconfig:option:`CONFIG_BT_MESH_LOW_POWER` option and related configuration options.

Replay Protection List
**********************

The Replay Protection List (RPL) is used to hold recently received sequence
numbers from elements within the mesh network to perform protection against
replay attacks.

To keep a node protected against replay attacks after reboot, it needs to store
the entire RPL in the persistent storage before it is powered off. Depending on
the amount of traffic in a mesh network, storing recently seen sequence numbers
can make flash wear out sooner or later. To mitigate this,
:kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT` can be used. This option postpones
storing of RPL entries in the persistent storage.

This option, however, doesn't completely solve the issue as the node may
get powered off before the timer to store the RPL is fired. To ensure that
messages can not be replayed, the node can initiate storage of the pending
RPL entry (or entries) at any time (or sufficiently before power loss)
by calling :c:func:`bt_mesh_rpl_pending_store`. This is up to the node to decide,
which RPL entries are to be stored in this case.

Setting :kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT` to -1 allows to completely
switch off the timer, which can help to significantly reduce flash wear out.
This moves the responsibility of storing RPL to the user application and
requires that sufficient power backup is available from the time this API
is called until all RPL entries are written to the flash.

Finding the right balance between :kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT` and
calling :c:func:`bt_mesh_rpl_pending_store` may reduce a risk of security
vulnerability and flash wear out.

.. warning:

   Failing to enable :kconfig:option:`CONFIG_BT_SETTINGS`, or setting
   :kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT` to -1 and not storing
   the RPL between reboots, will make the device vulnerable to replay attacks
   and not perform the replay protection required by the spec.


API reference
**************

.. doxygengroup:: bt_mesh
