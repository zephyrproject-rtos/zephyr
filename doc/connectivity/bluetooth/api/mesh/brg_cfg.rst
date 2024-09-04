.. _bluetooth_mesh_brg:

Subnet Bridge
#############

The Bluetooth Mesh Subnet Bridge feature allows a node to relay messages between different subnets in a Mesh network.

The Subnet Bridge feature includes two models:

.. toctree::
   :maxdepth: 1

   brg_cfg_srv
   brg_cfg_cli

The Bridge Configuration Server model is mandatory for supporting the Subnet Bridge feature.
The Bridge Configuration Client model is optional and allows nodes to configure the Subnet Bridge on other nodes.
These models define the necessary states, messages, and behaviors for configuring and managing the Subnet Bridge feature.

Concepts
********

The Subnet Bridge feature introduces several concepts to enable message relaying between different subnets in a Mesh network.

Subnet
======

A subnet is a group of nodes within a Mesh network that share a common network key, enabling them to communicate securely at the network layer.
Each subnet operates independently, with nodes exchanging messages exclusively within that group.
A node may belong to multiple subnets at the same time.

Subnet Bridge
=============

A Subnet Bridge is a node in a Mesh network that connects multiple subnets, allowing communication between them by relaying messages across subnet groups.
Only nodes with the Subnet Bridge functionality enabled can perform this role.
The Subnet Bridge node has a primary subnet, based on the primary NetKey, which handles the IV Update procedure and propagates updates to other subnets.
The secondary subnets, on which messages are relayed, are referred to as *bridged subnets*.
The Bridge Configuration Client can get the Subnet Bridge feature status of a node by sending a **Bridge Configuration Get** message to the Bridge Configuration Server model on the target node, using the :c:func:`bt_mesh_brg_cfg_cli_subnet_bridge_get` function.
The Client can also request to get the list of subnets that are currently bridged by a node by sending a **Bridged Subnets Get** message by calling the :c:func:`bt_mesh_brg_cfg_cli_bridged_subnets_get` function.

Bridging Table
==============

The Bridging Table contains the entries for the subnets that are brigded by the node, and is managed by the Bridge Configuration Server model.
The Bridge Configuration Client can get the Bridging Table status by sending a **Bridging Table Get** message to the Server model on the target node, calling the :c:func:`bt_mesh_brg_cfg_cli_bridging_table_get` function.

The maximum number of subnets a node can bridge is defined by the :kconfig:option:`BT_MESH_BRG_TABLE_ITEMS_MAX` option, which defaults to the minimum value of 16, with a maximum possible size of 255.
It is recommended to keep the number of bridged subnets as low as possible to minimize the impact on the network.
The Client can request the size of the Bridging Table from a Server by sending a **Bridging Table Size Get** message, using the :c:func:`bt_mesh_brg_cfg_cli_bridging_table_size_get` function.

Configuration and Management
****************************

The configuration and management of the Subnet Bridge feature are handled using the Bridge Configuration Server and Client models.
A node implementing the Bridge Configuration Client model can act as a *Configuration Manager* for the Subnet Bridge feature.

Enable or Disable the Subnet Bridge Feature
===========================================

The Bridge Configuration Client (or Configuration Manager) can enable or disable the Subnet Bridge feature on a node by sending a **Bridge Configuration Set** message to the Bridge Configuration Server model on the target node, using the :c:func:`bt_mesh_brg_cfg_cli_subnet_bridge_set` function.

Add or Remove Subnets
=====================

The Bridge Configuration Client can add or remove subnets from a node's Bridging Table by sending a **Bridge Configuration Add** or **Bridge Configuration Remove** message to the Bridge Configuration Server model on the target node, calling the :c:func:`bt_mesh_brg_cfg_cli_bridging_table_add` or :c:func:`bt_mesh_brg_cfg_cli_bridging_table_remove` functions.


API reference
*************

This section contains types and defines common to the Bridge Configuration models.

.. doxygengroup:: bt_mesh_brg
   :project: Zephyr
   :members:
