.. zephyr:code-sample:: wifi-mesh
   :name: Wi-Fi Mesh

   Build a self-organizing multi-hop Wi-Fi mesh network.

Overview
********

This sample forms a self-organizing Wi-Fi mesh network across a group of
ESP32 boards. Each node runs Wi-Fi in combined AP and station mode: the
station interface links upward to a parent node, and the softAP interface
accepts downstream children. The nodes elect a root among themselves. Only
the root node associates with the configured router to reach the external
network; internal nodes exchange data purely over the mesh with
``esp_wifi_mesh_send()`` and ``esp_wifi_mesh_recv()``.

The mesh is driven interactively through a ``mesh`` shell. Received mesh
messages are logged as they arrive.

This sample carries no IP: no node runs the network stack over the mesh, not
even the root's link to the router. It moves opaque application payloads
between nodes with ``esp_wifi_mesh_send()``/``esp_wifi_mesh_recv()`` only. It
fits applications that define their own light protocol over the mesh and do
not need sockets, DHCP or internet access, for example collecting sensor
readings toward the root or pushing commands out to the nodes.

For a node to reach the external network, or to run standard IP (sockets,
DHCP, mDNS) over the mesh, use the :zephyr:code-sample:`wifi-mesh-ip` sample
instead. There the root NATs child traffic out to the router, so every node
reaches the internet through the root, at the cost of the IP-over-mesh
constraints documented in that sample.

Shell commands
==============

.. list-table::
   :header-rows: 1

   * - Command
     - Description
   * - ``mesh status``
     - Show this node's layer, role (root or node), routing table size and
       toDS reachability.
   * - ``mesh send <text>``
     - Send a text message over the mesh. A non-root node sends toward the
       root; the root sends to each node in its routing table.
   * - ``mesh table``
     - Dump the routing table (the node addresses this node routes to).
   * - ``mesh autotx <on|off>``
     - Toggle a periodic background sender that emits ``hello seq=N`` every
       few seconds, off by default.

Requirements
************

* At least two supported ESP32 boards (esp32, esp32s2, esp32s3, esp32c3,
  esp32c5, esp32c6). ESP32-C2 and ESP32-H2 do not provide a mesh library.
* A 2.4 GHz router. In the self-organized mode used by this sample the root
  is elected by router reachability, so a router is required for the mesh to
  form even though this sample does not use the external network.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/wifi_mesh
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :compact:

Configure the mesh channel and the router credentials through Kconfig
(``CONFIG_WIFI_ESP32_MESH_CHANNEL``, ``CONFIG_WIFI_ESP32_MESH_ROUTER_SSID``,
``CONFIG_WIFI_ESP32_MESH_ROUTER_PSK``). The router credentials default to the
``myssid``/``mypassword`` placeholders and must be set to the router in use;
a node left on the placeholder finds no such network and keeps searching for
a parent. Set the channel to the router's channel. Flash the same image to
every board.

Node role
=========

The ``CONFIG_WIFI_ESP32_MESH_ROLE`` choice sets how a node takes its place in
the tree:

* ``CONFIG_WIFI_ESP32_MESH_ROLE_AUTO`` (default): self-organized. The node
  scans, and if it can reach the router it votes to become the root and
  connects to it; otherwise it joins an in-range mesh node as a child. A
  router is required, since the election is driven by router reachability.
* ``CONFIG_WIFI_ESP32_MESH_FORCE_ROOT``: this node is the fixed root. It never
  elects and always sits at the top of the tree. With a router configured it
  still associates to it and maintains that uplink; only in a router-less setup
  is the parent search stopped, since it would otherwise scan for a parent that
  does not exist.
* ``CONFIG_WIFI_ESP32_MESH_FORCE_CHILD``: this node never becomes root and
  attaches as a child.

A router is required in every role: children attach through the self-organized
parent search, which is anchored on the root's association to the router. A
fully router-less mesh would need each child to be given an explicit parent,
which this sample does not do.

In the auto role, because root election is driven by router reachability,
boards that are all within range of the router each become their own root; a
node joins the mesh as a child only when it is out of router range. For bench
testing with all boards near the router, build the intended child nodes with
``CONFIG_WIFI_ESP32_MESH_FORCE_CHILD=y`` so they attach to the elected root
instead of each becoming their own root.

To pin the topology on a bench, build one board with
``CONFIG_WIFI_ESP32_MESH_FORCE_ROOT=y`` and the rest with
``CONFIG_WIFI_ESP32_MESH_FORCE_CHILD=y``. The fixed root sits at layer 1 and
the children attach beneath it.

Sample Output
*************

.. code-block:: console

   <inf> wifi_mesh: Wi-Fi mesh starting, use the "mesh" shell commands
   <inf> wifi_mesh: mesh started, layer -1
   <inf> wifi_mesh: parent connected, layer 1 (root)
   <inf> wifi_mesh: child connected
   <inf> wifi_mesh: routing table size 1

   uart:~$ mesh status
   layer:         1
   role:          root
   routing table: 1 node(s)
   toDS:          unreachable
   autotx:        off
   uart:~$ mesh send hello-mesh
   sent 11 bytes over mesh
   uart:~$ mesh table
   routing table: 1 node(s)
    0: aa:bb:cc:dd:ee:ff

   <inf> wifi_mesh: rx 11 bytes: hello-mesh
