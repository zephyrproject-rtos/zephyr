.. _bluetooth_mesh_cdb_usage:

Configuration Database (CDB)
############################

.. contents::
   :local:
   :depth: 2

Overview
********

The Configuration Database (CDB) is the subsystem for Bluetooth® Mesh provisioner devices.
It stores and manages information about the mesh network, including provisioned nodes, network keys, and application keys.
The CDB enables a provisioner to track the network topology, assign addresses, and configure nodes systematically.

.. note::

   The CDB implementation has proprietary design and does not follow the Bluetooth SIG specification.

The CDB is enabled with the :kconfig:option:`CONFIG_BT_MESH_CDB` configuration option and is typically used only on the provisioner device in the network.
There should be only one device in the mesh network with CDB enabled to avoid conflicts.

Key features
============

* **Persistent Storage**: All CDB data is stored persistently when :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, allowing the provisioner to recover network state across reboots.
* **Address Management**: Automatic allocation and tracking of unicast addresses for provisioned nodes.
* **Key Management**: Storage and synchronization of network keys and application keys.
* **Node Tracking**: Maintains information about each provisioned node including UUID, address, element count, device key, and configuration status.
* **IV Index Management**: Tracks the network's IV Index and IV Update state.

Configuration options
*********************

The following Kconfig options control the CDB capacity:

* :kconfig:option:`CONFIG_BT_MESH_CDB_NODE_COUNT` - Maximum number of nodes the CDB can handle.
* :kconfig:option:`CONFIG_BT_MESH_CDB_SUBNET_COUNT` - Maximum number of subnets the CDB can handle.
* :kconfig:option:`CONFIG_BT_MESH_CDB_APP_KEY_COUNT` - Maximum number of application keys the CDB can handle.

Additional options:

* :kconfig:option:`CONFIG_BT_MESH_CDB_KEY_SYNC` - Enables automatic synchronization between mesh keys on the provisioner node and the CDB keys.
  If option is enabled then operations with keys performed through the :ref:`bluetooth_mesh_models_cfg_srv` are automatically mirrored to the CDB.

Data Structures
***************

The CDB was designed to handle data collected in several data structures.
The structures represent nodes, subnets, application keys, and the overall database state.

Node structure
==============

.. literalinclude:: ../../../../../include/zephyr/bluetooth/mesh/cdb.h
   :language: c
   :dedent:
   :start-after: doc string cdb node start
   :end-before: doc string cdb node end

Node flags:

* ``BT_MESH_CDB_NODE_CONFIGURED`` - Set when the node has been configured with keys and bindings.

Subnet structure
================

.. literalinclude:: ../../../../../include/zephyr/bluetooth/mesh/cdb.h
   :language: c
   :dedent:
   :start-after: doc string cdb subnet start
   :end-before: doc string cdb subnet end

Application Key structure
==========================

.. literalinclude:: ../../../../../include/zephyr/bluetooth/mesh/cdb.h
   :language: c
   :dedent:
   :start-after: doc string cdb app key start
   :end-before: doc string cdb app key end

Main CDB structure
==================

.. literalinclude:: ../../../../../include/zephyr/bluetooth/mesh/cdb.h
   :language: c
   :dedent:
   :start-after: doc string cdb start
   :end-before: doc string cdb end

Usage scenarios
***************

The following patterns demonstrate common CDB usage scenarios:

* :ref:`cdb_pattern_1` - Creating and initializing the CDB
* :ref:`cdb_pattern_2` - Managing node provisioning and addresses
* :ref:`cdb_pattern_3` - Managing network keys and subnets
* :ref:`cdb_pattern_4` - Finding and removing subnets
* :ref:`cdb_pattern_5` - Adding application keys to the CDB
* :ref:`cdb_pattern_6` - Finding and removing application keys
* :ref:`cdb_pattern_7` - Configuring provisioned nodes
* :ref:`cdb_pattern_8` - Processing nodes with callbacks
* :ref:`cdb_pattern_9` - Finding and removing nodes
* :ref:`cdb_pattern_10` - Managing IV Index updates
* :ref:`cdb_pattern_11` - Importing/exporting device keys
* :ref:`cdb_pattern_12` - Resetting the database

.. note::

   The provided below source code of scenarios should be treated as conceptual patterns rather than exact implementations.
   They require adaptation to fit into an application context and testing.
   Please read the CDB API description for correct usage.

.. _cdb_pattern_1:

Provisioner initialization
==========================

On the embedded provisioner node, the CDB must be initialized before the mesh stack is provisioned and the application can use it for key storage, node tracking, and configuration.
This step establishes the primary network key, the initial subnet, and the starting IV Index and address space, so that all later CDB operations work on a well‑defined network context and can be persisted across reboots.

A typical provisioner initializes the CDB during startup:

.. code-block:: c

   #include <zephyr/bluetooth/mesh.h>

   static int provisioner_init(void)
   {
       uint8_t net_key[16];
       int err;

       /* Initialize Bluetooth Mesh */
       err = bt_mesh_init(&prov, &comp);
       if (err) {
           return err;
       }

       /* Load settings if persistent storage is enabled */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           settings_load();
       }

       /* Generate or use predefined network key */
       bt_rand(net_key, 16);

       /* Create CDB with primary network key */
       err = bt_mesh_cdb_create(net_key);
       if (err == -EALREADY) {
           printk("Using stored CDB\n");
       } else if (err) {
           printk("Failed to create CDB (err %d)\n", err);
           return err;
       } else {
           printk("Created new CDB\n");
       }

       return 0;
   }

Key points:

* Call the :c:func:`bt_mesh_cdb_create` function with the primary network key.
* Returns error code if CDB already exists (loaded from persistent storage).
* The function automatically creates a subnet with ``NetIdx = 0``.
* Sets the IV Index to ``0`` and lowest available address to ``1``.

.. _cdb_pattern_2:

Provisioning and node allocation
================================

When you add new devices to the mesh, their addresses, device keys, and basic metadata must be recorded consistently in the provisioner’s database.
Automatic node allocation during PB‑ADV/PB‑GATT provisioning keeps CDB and the actual network in sync.
When provisioning a new device, the CDB automatically allocates the node during the provisioning process.

However, you can also manually allocate nodes or check allocation.
Manual node allocation is useful for advanced scenarios such as importing nodes from an external source, pre‑allocating address ranges, or recovering a network from known information.

.. code-block:: c

   /* Provisioning callback - node is automatically added to CDB */
   static void node_added(uint16_t idx, uint8_t uuid[16], uint16_t addr,
                          uint8_t num_elem)
   {
       printk("Node added: addr=0x%04x, elements=%d\n", addr, num_elem);
       /* The CDB node is created automatically by the provisioning subsystem */
   }

   static const struct bt_mesh_prov prov = {
       .uuid = dev_uuid,
       .node_added = node_added,
       /* ... other callbacks ... */
   };

   /* Manual node allocation (if needed) */
   static struct bt_mesh_cdb_node *allocate_node(const uint8_t uuid[16],
                                                   uint8_t num_elem)
   {
       struct bt_mesh_cdb_node *node;
       uint16_t addr;

       /* Get free address or specify one (0 = auto-allocate) */
       addr = bt_mesh_cdb_free_addr_get(num_elem);
       if (addr == BT_MESH_ADDR_UNASSIGNED) {
           printk("No free addresses available\n");
           return NULL;
       }

       /* Allocate node in CDB */
       node = bt_mesh_cdb_node_alloc(uuid, addr, num_elem, net_idx);
       if (node == NULL) {
           printk("Failed to allocate node\n");
           return NULL;
       }

       return node;
   }

Key points:

* During normal PB-ADV/PB-GATT provisioning, nodes are automatically added to the CDB.
* The :c:func:`bt_mesh_cdb_node_alloc` function creates a CDB entry with the specified parameters.
* Pass :c:macro:`BT_MESH_ADDR_UNASSIGNED` as the address to let the CDB auto-assign the lowest available address.
* The :c:func:`bt_mesh_cdb_free_addr_get` function finds a free address range for a given element count.
* Address allocation checks for conflicts and ensures the unicast address validity.

.. _cdb_pattern_3:

Subnet and key management
=========================

Large or segmented mesh networks often use multiple subnets to separate traffic domains, enable secure migration, or support the Key Refresh procedure.
Managing subnets and their network keys in the CDB allows the provisioner to create new subnets, rotate keys, and derive the correct flags and IV Index when provisioning and configuring nodes.

Manage network keys and subnets in the CDB:

.. code-block:: c

   /* Add a new subnet */
   static int add_subnet(uint16_t net_idx)
   {
       struct bt_mesh_cdb_subnet *sub;
       uint8_t net_key[16];
       int err;

       /* Allocate subnet in CDB */
       sub = bt_mesh_cdb_subnet_alloc(net_idx);
       if (sub == NULL) {
           printk("Failed to allocate subnet\n");
           return -ENOMEM;
       }

       /* Generate network key */
       bt_rand(net_key, 16);

       /* Import key value */
       err = bt_mesh_cdb_subnet_key_import(sub, 0, net_key);
       if (err) {
           bt_mesh_cdb_subnet_del(sub, false);
           return err;
       }

       /* Store subnet */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           bt_mesh_cdb_subnet_store(sub);
       }

       return 0;
   }

   /* Get subnet for provisioning */
   static int get_subnet_flags(uint16_t net_idx)
   {
       struct bt_mesh_cdb_subnet *sub;
       uint8_t flags;

       sub = bt_mesh_cdb_subnet_get(net_idx);
       if (sub == NULL) {
           return -ENOENT;
       }

       flags = bt_mesh_cdb_subnet_flags(sub);
       printk("Subnet flags: KR=%d IVU=%d\n",
              !!(flags & BT_MESH_NET_FLAG_KR),
              !!(flags & BT_MESH_NET_FLAG_IVU));

       return 0;
   }

Key points:

* The :c:func:`bt_mesh_cdb_subnet_alloc` function allocates a subnet by NetIdx.
* The :c:func:`bt_mesh_cdb_subnet_key_import` function sets the actual key value.
* The :c:func:`bt_mesh_cdb_subnet_flags` function returns flags needed for provisioning data.
* The flags include Key Refresh and IV Update state from the CDB.

.. _cdb_pattern_4:

Subnet lookup and removal
=========================

At some point you may need to inspect an existing subnet (for diagnostics or tooling) or remove it entirely (for example when decommissioning a segment, rotating to a new subnet, or cleaning up after experimentation).
The CDB provides lookup and deletion helpers so that the provisioner can keep its view of active subnets consistent with what nodes actually use.

Find and manage subnets in the CDB:

.. code-block:: c

   /* Get subnet by NetIdx */
   static void lookup_subnet(uint16_t net_idx)
   {
       struct bt_mesh_cdb_subnet *sub;

       sub = bt_mesh_cdb_subnet_get(net_idx);
       if (sub == NULL) {
           printk("Subnet 0x%03x not found\n", net_idx);
           return;
       }

       printk("Subnet 0x%03x: kr_phase=%d\n", net_idx, sub->kr_phase);
   }

   /* Remove a subnet from the CDB */
   static void remove_subnet(uint16_t net_idx)
   {
       struct bt_mesh_cdb_subnet *sub;

       sub = bt_mesh_cdb_subnet_get(net_idx);
       if (sub == NULL) {
           printk("Subnet not found\n");
           return;
       }

       /* First, remove the subnet from all nodes in the network */
       /* ... send NetKey Delete to all nodes using Config Client ... */

       /* Delete subnet from CDB */
       bt_mesh_cdb_subnet_del(sub, true);

       printk("Subnet 0x%03x removed\n", net_idx);
   }

   /* Export subnet key for external use */
   static int export_subnet_key(uint16_t net_idx, int key_idx)
   {
       struct bt_mesh_cdb_subnet *sub;
       uint8_t net_key[16];
       int err;

       sub = bt_mesh_cdb_subnet_get(net_idx);
       if (sub == NULL) {
           return -ENOENT;
       }

       err = bt_mesh_cdb_subnet_key_export(sub, key_idx, net_key);
       if (err) {
           printk("Failed to export subnet key (err %d)\n", err);
           return err;
       }

       printk("NetKey[%d] for subnet 0x%03x: ", key_idx, net_idx);
       for (int i = 0; i < 16; i++) {
           printk("%02x", net_key[i]);
       }
       printk("\n");

       return 0;
   }

Key points:

* The :c:func:`bt_mesh_cdb_subnet_get` function retrieves a subnet by its NetKeyIndex.
* Pass ``true`` to the :c:func:`bt_mesh_cdb_subnet_del` function to clear persistent storage.
* Always remove the subnet from all nodes before deleting it from the CDB.
* Use the :c:func:`bt_mesh_cdb_subnet_key_export` function to retrieve key material securely.
* The ``key_idx`` parameter (``0`` or ``1``) selects between old and new keys during the Key Refresh procedure.

.. _cdb_pattern_5:

Setting up application keys
===========================

Application keys define which application traffic can be encrypted and where it can be bound.
After the CDB is initialized, the provisioner must create one or more application keys so that it can bind models on nodes and enable real application‑level communication (for example, lighting or sensor data).

After creating the CDB, add application keys for node configuration:

.. code-block:: c

   static void setup_cdb_keys(void)
   {
       struct bt_mesh_cdb_app_key *key;
       uint8_t app_key[16];
       int err;

       /* Allocate application key in CDB */
       key = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
       if (key == NULL) {
           printk("Failed to allocate app-key\n");
           return;
       }

       /* Generate random key value */
       bt_rand(app_key, 16);

       /* Import the key into CDB */
       err = bt_mesh_cdb_app_key_import(key, 0, app_key);
       if (err) {
           printk("Failed to import appkey (err %d)\n", err);
           return;
       }

       /* Store to persistent storage */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           bt_mesh_cdb_app_key_store(key);
       }
   }

Key points:

* The :c:func:`bt_mesh_cdb_app_key_alloc` function allocates a slot in the CDB.
* The :c:func:`bt_mesh_cdb_app_key_import` function sets the actual key value.
* The second parameter to import (``0``) is the key index for the Key Refresh procedure (``0`` = current key).
* Always store keys after creation when using persistent storage.

.. _cdb_pattern_6:

Application keys lookup and removal
===================================

Over time, you may need to inspect which network key an application key is bound to, rotate or revoke application keys, or clean up keys that are no longer used.
Proper lookup and removal through the CDB helps avoid dangling bindings and ensures that nodes and the provisioner share the same view of usable application keys.

Find and manage application keys in the CDB:

.. code-block:: c

   /* Get application key by AppIdx */
   static void lookup_app_key(uint16_t app_idx)
   {
       struct bt_mesh_cdb_app_key *key;

       key = bt_mesh_cdb_app_key_get(app_idx);
       if (key == NULL) {
           printk("AppKey 0x%03x not found\n", app_idx);
           return;
       }

       printk("AppKey 0x%03x: bound to NetIdx 0x%03x\n",
              app_idx, key->net_idx);
   }

   /* Remove an application key from the CDB */
   static void remove_app_key(uint16_t app_idx)
   {
       struct bt_mesh_cdb_app_key *key;

       key = bt_mesh_cdb_app_key_get(app_idx);
       if (key == NULL) {
           printk("AppKey not found\n");
           return;
       }

       /* First, remove the app key from all nodes in the network */
       /* ... send AppKey Delete to all nodes using Config Client ... */

       /* Delete app key from CDB */
       bt_mesh_cdb_app_key_del(key, true);

       printk("AppKey 0x%03x removed\n", app_idx);
   }

   /* Export application key for external use */
   static int export_app_key(uint16_t app_idx, int key_idx)
   {
       struct bt_mesh_cdb_app_key *key;
       uint8_t app_key[16];
       int err;

       key = bt_mesh_cdb_app_key_get(app_idx);
       if (key == NULL) {
           return -ENOENT;
       }

       err = bt_mesh_cdb_app_key_export(key, key_idx, app_key);
       if (err) {
           printk("Failed to export app key (err %d)\n", err);
           return err;
       }

       printk("AppKey[%d] 0x%03x: ", key_idx, app_idx);
       for (int i = 0; i < 16; i++) {
           printk("%02x", app_key[i]);
       }
       printk("\n");

       return 0;
   }

   /* Update application key binding */
   static int update_app_key_binding(uint16_t app_idx, uint16_t new_net_idx)
   {
       struct bt_mesh_cdb_app_key *key;

       key = bt_mesh_cdb_app_key_get(app_idx);
       if (key == NULL) {
           return -ENOENT;
       }

       /* Verify the new subnet exists */
       if (bt_mesh_cdb_subnet_get(new_net_idx) == NULL) {
           printk("Target subnet 0x%03x not found\n", new_net_idx);
           return -ENOENT;
       }

       key->net_idx = new_net_idx;

       /* Store updated binding */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           bt_mesh_cdb_app_key_store(key);
       }

       printk("AppKey 0x%03x rebound to NetIdx 0x%03x\n",
              app_idx, new_net_idx);

       return 0;
   }

Key points:

* The :c:func:`bt_mesh_cdb_app_key_get` function retrieves an application key by its AppKeyIndex.
* Pass ``true`` to the :c:func:`bt_mesh_cdb_app_key_del` function to clear persistent storage.
* Always remove the app key from all nodes before deleting it from the CDB.
* Use the :c:func:`bt_mesh_cdb_app_key_export` function to retrieve key material securely.
* The ``key_idx`` parameter (``0`` or ``1``) selects between old and new keys during the Key Refresh procedure.
* Application keys are bound to a network key through the ``net_idx`` field.

.. _cdb_pattern_7:

Node configuration
==================

Provisioning only adds a node to the network; it does not make it participate in your application.
After provisioning, you must configure nodes—add network/app keys, bind models, and set subscriptions/publications.
The CDB stores the keys and configuration status so the provisioner can resume configuration after a reboot and avoid re‑configuring already finished nodes.

After provisioning, configure the node by adding keys and binding models:

.. code-block:: c

   static void configure_node(struct bt_mesh_cdb_node *node)
   {
       NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
       struct bt_mesh_cdb_app_key *key;
       uint8_t app_key[16];
       uint8_t status;
       int err;

       printk("Configuring node 0x%04x\n", node->addr);

       /* Get application key from CDB */
       key = bt_mesh_cdb_app_key_get(app_idx);
       if (key == NULL) {
           printk("App-key not found\n");
           return;
       }

       /* Export key value from CDB */
       err = bt_mesh_cdb_app_key_export(key, 0, app_key);
       if (err) {
           printk("Failed to export key (err %d)\n", err);
           return;
       }

       /* Add application key to the node */
       err = bt_mesh_cfg_cli_app_key_add(net_idx, node->addr,
                                         net_idx, app_idx,
                                         app_key, &status);
       if (err || status) {
           printk("Failed to add app-key (err %d, status %d)\n",
                  err, status);
           return;
       }

       /* Get composition data and bind models */
       err = bt_mesh_cfg_cli_comp_data_get(net_idx, node->addr, 0,
                                           &status, &buf);
       if (err || status) {
           printk("Failed to get composition data\n");
           return;
       }

       /* Parse composition and bind models to app key */
       /* ... model binding code ... */

       /* Mark node as configured */
       atomic_set_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED);

       /* Persist configuration status */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           bt_mesh_cdb_node_store(node);
       }
   }

Key points:

* Use the :c:func:`bt_mesh_cdb_app_key_get` and the :c:func:`bt_mesh_cdb_app_key_export` functions to retrieve keys.
* The Configuration Client model is used to configure remote nodes.
* Set the ``BT_MESH_CDB_NODE_CONFIGURED`` flag after successful configuration.
* Always call the :c:func:`bt_mesh_cdb_node_store` function to persist the configured state.

.. _cdb_pattern_8:

Iterating over nodes
====================

Many management tasks involve acting on all (or a filtered subset of) nodes: checking which ones still need configuration, generating statistics, or performing bulk operations.
The CDB iterator lets the provisioner traverse all known nodes in a safe, abstract way without relying on internal storage details.

The CDB provides an iterator to process all allocated nodes:

.. code-block:: c

   /* Check for unconfigured nodes */
   static uint8_t check_unconfigured(struct bt_mesh_cdb_node *node,
                                      void *user_data)
   {
       if (!atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
           printk("Node 0x%04x needs configuration\n", node->addr);
           configure_node(node);
       }

       return BT_MESH_CDB_ITER_CONTINUE;
   }

   static void process_nodes(void)
   {
       bt_mesh_cdb_node_foreach(check_unconfigured, NULL);
   }

   /* Example: Count nodes on a specific subnet */
   static uint8_t count_subnet_nodes(struct bt_mesh_cdb_node *node,
                                      void *user_data)
   {
       uint16_t *net_idx = user_data;
       static int count = 0;

       if (node->net_idx == *net_idx) {
           count++;
       }

       return BT_MESH_CDB_ITER_CONTINUE;
   }

Key points:

* The :c:func:`bt_mesh_cdb_node_foreach` function calls the callback for each allocated node.
* Return ``BT_MESH_CDB_ITER_CONTINUE`` to continue iteration.
* Return ``BT_MESH_CDB_ITER_STOP`` to stop iteration early.
* The callback is only called for nodes with ``addr != BT_MESH_ADDR_UNASSIGNED``.

.. _cdb_pattern_9:

Node lookup and removal
=======================

Sometimes a single node must be inspected (for example, when debugging) or removed from the network (for example, when it is physically decommissioned or replaced).
Using CDB node lookup and deletion keeps the logical view of the mesh consistent with the actual devices and prevents address reuse conflicts.

Find and manage nodes in the CDB:

.. code-block:: c

   /* Get node by address */
   static void lookup_node(uint16_t addr)
   {
       struct bt_mesh_cdb_node *node;

       node = bt_mesh_cdb_node_get(addr);
       if (node == NULL) {
           printk("Node 0x%04x not found\n", addr);
           return;
       }

       printk("Node 0x%04x: %d elements, net_idx=0x%03x\n",
              node->addr, node->num_elem, node->net_idx);
   }

   /* Remove a node from the network */
   static void remove_node(uint16_t addr)
   {
       struct bt_mesh_cdb_node *node;

       node = bt_mesh_cdb_node_get(addr);
       if (node == NULL) {
           printk("Node not found\n");
           return;
       }

       /* First, send Node Reset to the device (recommended) */
       /* ... send reset command using Config Client ... */

       /* Delete node from CDB */
       bt_mesh_cdb_node_del(node, true);

       printk("Node 0x%04x removed\n", addr);
   }

Key points:

* The :c:func:`bt_mesh_cdb_node_get` function searches by element address (works for any element address).
* Always send a ``Node Reset`` command before removing a node to avoid orphaned devices.
* Pass ``true`` to the :c:func:`bt_mesh_cdb_node_del` function to clear persistent storage.
* Deleting a node might update the ``lowest_avail_addr`` if appropriate (please read CDB API for details).

.. _cdb_pattern_10:

IV Index management
===================

The IV Index is a core part of Bluetooth Mesh security and replay protection.
When IV Update procedures occur, the provisioner must update its stored IV Index so that future provisioning and configuration use the correct value and address space.
Storing this in the CDB keeps the network state coherent across reboots and between provisioning actions.

Update the network's IV Index:

.. code-block:: c

   /* Update IV Index when IV Update procedure occurs */
   static void handle_iv_update(uint32_t iv_index, bool iv_update)
   {
       bt_mesh_cdb_iv_update(iv_index, iv_update);

       printk("IV Index updated: %u (IV Update: %s)\n",
              iv_index, iv_update ? "in progress" : "normal");
   }

   /* The IV Index is automatically used during provisioning */
   static void provision_with_current_iv(void)
   {
       /* The provisioner automatically uses bt_mesh_cdb.iv_index
        * and bt_mesh_cdb_subnet_flags() when sending provisioning data
        */
   }

Key points:

* The :c:func:`bt_mesh_cdb_iv_update` function updates both the IV Index and IV Update flag.
* The CDB automatically resets ``lowest_avail_addr`` during IV Index updates.
* The provisioner subsystem automatically uses CDB's IV Index for new nodes.

.. _cdb_pattern_11:

Working with device keys
========================

Device keys are sensitive information and often handled through secure key storage (for example, PSA crypto).
The CDB’s key import/export APIs abstract how keys are stored, allowing the application to use device keys when needed (for configuration or external tooling) without breaking the security model or relying on direct pointers into key storage.

Import and export device keys using the key management API:

.. code-block:: c

   /* Import a known device key */
   static int import_dev_key(struct bt_mesh_cdb_node *node,
                             const uint8_t dev_key[16])
   {
       int err;

       err = bt_mesh_cdb_node_key_import(node, dev_key);
       if (err) {
           printk("Failed to import device key (err %d)\n", err);
           return err;
       }

       /* Store updated node */
       if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
           bt_mesh_cdb_node_store(node);
       }

       return 0;
   }

   /* Export device key for external use */
   static int export_dev_key(struct bt_mesh_cdb_node *node)
   {
       uint8_t dev_key[16];
       int err;

       err = bt_mesh_cdb_node_key_export(node, dev_key);
       if (err) {
           printk("Failed to export device key (err %d)\n", err);
           return err;
       }

       /* Use the exported key */
       printk("Device key: ");
       for (int i = 0; i < 16; i++) {
           printk("%02x", dev_key[i]);
       }
       printk("\n");

       return 0;
   }

Key points:

* Always use import/export functions when working with keys (required for PSA crypto).
* Keys are stored securely and are not be directly accessible through pointers.
* The provisioner automatically generates and imports device keys during provisioning.

.. _cdb_pattern_12:

Clearing the CDB
================

For factory reset, testing, or starting a completely new network, you may need to wipe all provisioning and configuration data.
Clearing the CDB resets the logical network state so you can safely recreate it from scratch, while avoiding inconsistencies with stale keys or node entries.

Reset the entire configuration database:

.. code-block:: c

   static void reset_network(void)
   {
       printk("Clearing CDB...\n");

       /* Clear all CDB data */
       bt_mesh_cdb_clear();

       printk("CDB cleared\n");
   }

Key points:

* The :c:func:`bt_mesh_cdb_clear` function removes all nodes, subnets, and app keys.
* Clears persistent storage if the :kconfig:option:`CONFIG_BT_SETTINGS` Kconfig option is enabled.
* Marks the CDB as invalid (must call the :c:func:`bt_mesh_cdb_create` function to use again).

Best practices
**************

* **Error handling** - Check return values from all CDB functions, especially allocation functions which can fail when capacity is reached.

* **Address management** - Let the CDB manage address allocation by passing ``0`` to the :c:func:`bt_mesh_cdb_node_alloc` function unless you have specific addressing requirements.

* **Configuration tracking** - Always set the ``BT_MESH_CDB_NODE_CONFIGURED`` flag and store the node after successful configuration to avoid reconfiguring nodes unnecessarily.

* **Key security** - Never log or expose keys in production code.
  Use import/export functions to handle key material securely.

* **Iteration safety** - Do not modify the CDB (add/remove nodes) during :c:func:`bt_mesh_cdb_node_foreach` iteration.
  Collect addresses first, then modify.

Common pitfalls
***************

CDB synchronization among multiple provisioners
   If there are multiple provisioners in the network, ensure they synchronize their CDBs to avoid conflicts.
   Use application-level mechanisms to share CDB among multiple provisioners.
   There is no built-in mechanism for CDB synchronization.

Forgetting to store
   CDB modifications are not automatically persisted.
   Always call the appropriate ``_store()`` function after making changes when using persistent storage.

Incorrect key index
   When importing/exporting keys, the key index parameter (``0`` or ``1``) refers to the position in the Key Refresh key array, not the NetIdx or AppIdx.

Address conflicts
   If manually specifying addresses, ensure they do not conflict with existing nodes.
   Use the :c:func:`bt_mesh_cdb_free_addr_get` function to find available addresses.

Over-capacity
   CDB has fixed capacity defined by Kconfig.
   Exceeding capacity causes allocation failures.
   Monitor CDB usage and increase capacity if needed.

Accessing invalid nodes
   Always check if returned pointers from ``_get()`` or ``_alloc()`` functions are ``NULL`` before dereferencing.

API reference
*************

For complete API documentation, refer to:

* Header file: :file:`include/zephyr/bluetooth/mesh/cdb.h`
* Implementation: :file:`subsys/bluetooth/mesh/cdb.c`

Related documentation
*********************

* :ref:`bluetooth_mesh`
* :ref:`bluetooth_mesh_provisioning`
* Mesh Shell commands: :ref:`bluetooth_mesh_shell`
