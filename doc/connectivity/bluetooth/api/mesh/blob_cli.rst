.. _bluetooth_mesh_blob_cli:

BLOB Transfer Client
####################

The Binary Large Object (BLOB) Transfer Client is the sender of the BLOB transfer. It supports
sending BLOBs of any size to any number of Target nodes, in both Push BLOB Transfer Mode and Pull
BLOB Transfer Mode.

Usage
*****

Initialization
==============

The BLOB Transfer Client is instantiated on an element with a set of event handler callbacks:

.. code-block:: C

   static const struct bt_mesh_blob_cli_cb blob_cb = {
         /* Callbacks */
   };

   static struct bt_mesh_blob_cli blob_cli = {
         .cb = &blob_cb,
   };

   static struct bt_mesh_model models[] = {
         BT_MESH_MODEL_BLOB_CLI(&blob_cli),
   };

Transfer context
================

Both the transfer capabilities retrieval procedure and the BLOB transfer uses an instance of a
:c:type:`bt_mesh_blob_cli_inputs` to determine how to perform the transfer. The BLOB Transfer Client
Inputs structure must at least be initialized with a list of targets, an application key and a time
to live (TTL) value before it is used in a procedure:

.. code-block:: c

   static struct bt_mesh_blob_target targets[3] = {
           { .addr = 0x0001 },
           { .addr = 0x0002 },
           { .addr = 0x0003 },
   };
   static struct bt_mesh_blob_cli_inputs inputs = {
           .app_idx = MY_APP_IDX,
           .ttl = BT_MESH_TTL_DEFAULT,
   };

   sys_slist_init(&inputs.targets);
   sys_slist_append(&inputs.targets, &targets[0].n);
   sys_slist_append(&inputs.targets, &targets[1].n);
   sys_slist_append(&inputs.targets, &targets[2].n);

Note that all BLOB Transfer Servers in the transfer must be bound to the chosen application key.


Group address
-------------

The application may additionally specify a group address in the context structure. If the group is
not :c:macro:`BT_MESH_ADDR_UNASSIGNED`, the messages in the transfer will be sent to the group
address, instead of being sent individually to each Target node. Mesh Manager must ensure that all
Target nodes having the BLOB Transfer Server model subscribe to this group address.

Using group addresses for transferring the BLOBs can generally increase the transfer speed, as the
BLOB Transfer Client sends each message to all Target nodes at the same time. However, sending
large, segmented messages to group addresses in Bluetooth mesh is generally less reliable than
sending them to unicast addresses, as there is no transport layer acknowledgment mechanism for
groups. This can lead to longer recovery periods at the end of each block, and increases the risk of
losing Target nodes. Using group addresses for BLOB transfers will generally only pay off if the
list of Target nodes is extensive, and the effectiveness of each addressing strategy will vary
heavily between different deployments and the size of the chunks.

Transfer timeout
----------------

If a Target node fails to respond to an acknowledged message within the BLOB Transfer Client's time
limit, the Target node is dropped from the transfer. The application can reduce the chances of this
by giving the BLOB Transfer Client extra time through the context structure. The extra time may be
set in 10-second increments, up to 182 hours, in addition to the base time of 20 seconds. The wait
time scales automatically with the transfer TTL.

Note that the BLOB Transfer Client only moves forward with the transfer in following cases:

* All Target nodes have responded.
* A node has been removed from the list of Target nodes.
* The BLOB Transfer Client times out.

Increasing the wait time will increase this delay.

BLOB transfer capabilities retrieval
====================================

It is generally recommended to retrieve BLOB transfer capabilities before starting a transfer. The
procedure populates the transfer capabilities from all Target nodes with the most liberal set of
parameters that allows all Target nodes to participate in the transfer. Any Target nodes that fail
to respond, or respond with incompatible transfer parameters, will be dropped.

Target nodes are prioritized according to their order in the list of Target nodes. If a Target node
is found to be incompatible with any of the previous Target nodes, for instance by reporting a
non-overlapping block size range, it will be dropped. Lost Target nodes will be reported through the
:c:member:`lost_target <bt_mesh_blob_cli_cb.lost_target>` callback.

The end of the procedure is signalled through the :c:member:`caps <bt_mesh_blob_cli_cb.caps>`
callback, and the resulting capabilities can be used to determine the block and chunk sizes required
for the BLOB transfer.

BLOB transfer
=============

The BLOB transfer is started by calling :c:func:`bt_mesh_blob_cli_send` function, which (in addition
to the aforementioned transfer inputs) requires a set of transfer parameters and a BLOB stream
instance. The transfer parameters include the 64-bit BLOB ID, the BLOB size, the transfer mode, the
block size in logarithmic representation and the chunk size. The BLOB ID is application defined, but
must match the BLOB ID the BLOB Transfer Servers have been started with.

The transfer runs until it either completes successfully for at least one Target node, or it is
cancelled. The end of the transfer is communicated to the application through the :c:member:`end
<bt_mesh_blob_cli_cb.end>` callback. Lost Target nodes will be reported through the
:c:member:`lost_target <bt_mesh_blob_cli_cb.lost_target>` callback.

API reference
*************

.. doxygengroup:: bt_mesh_blob_cli
   :project: Zephyr
   :members:
