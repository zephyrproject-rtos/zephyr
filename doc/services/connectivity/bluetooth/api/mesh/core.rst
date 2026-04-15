.. _bluetooth_mesh_core:

Core
####

The core provides functionality for managing the general Bluetooth Mesh
state.

.. _bluetooth_mesh_lpn:

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

When using the LPN feature with logging, it is strongly recommended to only use
the :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` option. Log modes other than the
deferred may cause unintended delays during processing of log messages. This in
turns will affect scheduling of the receive delay and receive window. The same
limitation applies for the :kconfig:option:`CONFIG_BT_MESH_FRIEND` option.

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

.. _bluetooth_mesh_persistent_storage:

Persistent storage
******************

The mesh stack uses the :ref:`Settings Subsystem <settings_api>` for storing the
device configuration persistently. When the stack configuration changes and
the change needs to be stored persistently, the stack schedules a work item.
The delay between scheduling the work item and submitting it to the workqueue
is defined by the :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT` option. Once
storing of data is scheduled, it can not be rescheduled until the work item is
processed. Exceptions are made in certain cases as described below.

When IV index, Sequence Number or CDB configuration have to be stored, the work
item is submitted to the workqueue without the delay. If the work item was
previously scheduled, it will be rescheduled without the delay.

The Replay Protection List uses the same work item to store RPL entries. If
storing of RPL entries is requested and no other configuration is pending to be
stored, the delay is set to :kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT`.
If other stack configuration has to be stored, the delay defined by
the :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT` option is less than
:kconfig:option:`CONFIG_BT_MESH_RPL_STORE_TIMEOUT`, and the work item was
scheduled by the Replay Protection List, the work item will be rescheduled.

When the work item is running, the stack will store all pending configuration,
including the RPL entries.

Work item execution context
===========================

The :kconfig:option:`CONFIG_BT_MESH_SETTINGS_WORKQ` option configures the
context from which the work item is executed. This option is enabled by
default, and results in stack using a dedicated cooperative thread to
process the work item. This allows the stack to process other incoming and
outgoing messages, as well as other work items submitted to the system
workqueue, while the stack configuration is being stored.

When this option is disabled, the work item is submitted to the system workqueue.
This means that the system workqueue is blocked for the time it takes to store
the stack's configuration. It is not recommended to disable this option as this
will make the device non-responsive for a noticeable amount of time.

.. _bluetooth_mesh_adv_identity:

Advertisement identity
**********************

All mesh stack bearers advertise data with the :c:macro:`BT_ID_DEFAULT` local identity.
The value is preset in the mesh stack implementation. When Bluetooth® Low Energy (LE)
and Bluetooth Mesh coexist on the same device, the application should allocate and
configure another local identity for Bluetooth LE purposes before starting the communication.

API reference
**************

.. doxygengroup:: bt_mesh
