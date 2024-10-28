.. _bluetooth_mesh_brg_cfg:

Subnet Bridge
#############

With Bluetooth Mesh Protocol Specification version 1.1, the Bluetooth Mesh Subnet Bridge feature was
introduced.
This feature allows mesh networks to use subnets for area isolation, but to also allow communication
between specific devices in different, adjacent subnets without compromising security.

The Bluetooth Mesh Subnet Bridge feature makes it possible for selected nodes in the network to act
as Subnet Bridges, allowing controlled communication by relaying messages between nodes in adjacent
subnets.

The Subnet Bridge feature includes two models:

- :ref:`bluetooth_mesh_models_brg_cfg_srv`
- :ref:`bluetooth_mesh_models_brg_cfg_cli`

The Bridge Configuration Server model is mandatory for supporting the Subnet Bridge feature.
The Bridge Configuration Client model is optional and allows nodes to configure the Subnet Bridge on
other nodes.
These models define the necessary states, messages, and behaviors for configuring and managing the
Subnet Bridge feature.

The configuration and management of the Subnet Bridge feature are handled using the Bridge
Configuration Server and Client models.
A node implementing the Bridge Configuration Client model can act as a *Configuration Manager* for
the Subnet Bridge feature.

Concepts
********

To get a better understanding of the Subnet Bridge feature and its capabilities, an overview of a
few concepts is needed.

Subnet
======

A subnet is a group of nodes within a mesh network that share a common network key, enabling them to
communicate securely at the network layer.
Each subnet operates independently, with nodes exchanging messages exclusively within that group.
A node may belong to multiple subnets at the same time.

Subnet Bridge node
==================

A Subnet Bridge node is a node in a Bluetooth Mesh network that belongs to multiple subnets and has
a Subnet Bridge functionality enabled. Subnet bridging can be performed only by such a node. The
Subnet Bridge node connects the subnets, and allows communication between them by relaying
messages across the subnet groups.

The Subnet Bridge node has a primary subnet, based on the primary NetKey, which handles the
IV Update procedure and propagates updates to other subnets.
The secondary subnets, on which messages are relayed, are referred to as *bridged subnets*.

Bridging Table
==============

The Bridging Table contains the entries for the subnets that are bridged by the node, and is managed
by the Bridge Configuration Server model.

The maximum number of entries in the bridging table is defined by the
:kconfig:option:`CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX` option, which defaults to the minimum value of
16, with a maximum possible size of 255.

Enabling or disabling the Subnet Bridge feature
***********************************************

The Bridge Configuration Client (or Configuration Manager) can enable or disable the Subnet Bridge
feature on a node by sending a **Subnet Bridge Set** message to the Bridge Configuration
Server model on the target node, using the :c:func:`bt_mesh_brg_cfg_cli_set` function.

Adding or removing subnets
**************************

The Bridge Configuration Client can add or remove an entry from the Bridging Table by sending a
**Bridging Table Add** or **Bridging Table Remove** message to the Bridge Configuration
Server model on the target node, calling the :c:func:`bt_mesh_brg_cfg_cli_table_add` or
:c:func:`bt_mesh_brg_cfg_cli_table_remove` functions.

.. _bluetooth_mesh_brg_cfg_states:

Subnet Bridge states
********************

The Subnet Bridge has the following states:

- *Subnet Bridge*: This state indicates whether the Subnet Bridge feature is enabled or disabled on
  the node.
  The Bridge Configuration Client can retrieve this information by sending a **Subnet Bridge Get**
  message to the Bridge Configuration Server using the :c:func:`bt_mesh_brg_cfg_cli_get` function.

- *Bridging Table*: This state holds the bridging table. The Client can request a list of
  entries from a Bridging Table by sending a **Bridging Table Get** message to the target node using
  the :c:func:`bt_mesh_brg_cfg_cli_table_get` function.

  The Client can get a list of subnets currently bridged by a Subnet Bridge by sending a
  **Bridged Subnets Get** message to the target Server by calling the
  :c:func:`bt_mesh_brg_cfg_cli_subnets_get` function.

- *Bridging Table Size*: This state reports the maximum number of entries the Bridging Table can
  store. The Client can retrieve this information by sending a **Bridging Table Size Get** message
  using the :c:func:`bt_mesh_brg_cfg_cli_table_size_get` function.
  This is a read-only state.

Subnet bridging and replay protection
*************************************

The Subnet Bridge feature enables message relaying between subnets and requires effective replay
protection to ensure network security. Key considerations to take into account are described below.

Relay buffer considerations
===========================

When a message is relayed between subnets by a Subnet Bridge, it is allocated from the relay buffer.
To ensure that messages can be retransmitted to all subnetworks,
the :kconfig:option:`CONFIG_BT_MESH_RELAY_BUF_COUNT` option should be increased accordingly.

However, if the :kconfig:option:`CONFIG_BT_MESH_RELAY` feature is disabled, the messages will be
allocated from the advertising buffer instead. In this case, increase the
:kconfig:option:`CONFIG_BT_MESH_ADV_BUF_COUNT` option to allow for sufficient buffer space.

Replay protection and Bridging Table
====================================

A Subnet Bridge node must implement replay protection for all Access and Transport Control messages
sent to bridged subnets.

The Replay Protection List (RPL) works with the Bridging Table to ensure security:

- The Subnet Bridge stores the latest IVISeq for each source address authorized to send messages to
  bridged subnets.

- Messages with an IVISeq less than or equal to the stored value are discarded, while valid messages
  update the stored IVISeq before being relayed.

To ensure proper operation, it is important that the RPL and Bridging Table are synchronized,
as every bridged message must pass through the replay protection mechanism before being relayed.

.. note::

   The RPL size should scale with the Bridging Table. As the number of bridged subnets grows,
   more source addresses and IVISeq values must be tracked, requiring a larger RPL to maintain
   effective replay protection.

Subnet Bridge and Directed Forwarding
*************************************

Bluetooth Mesh Directed Forwarding (MDF) enables efficient routing between nodes across subnets by
optimizing relay paths. While MDF can enhance Subnet Bridging by handling path discovery and
forwarding, the current implementation does not support this feature.

API reference
*************

This section contains types and defines common to the Bridge Configuration models.

.. doxygengroup:: bt_mesh_brg_cfg
