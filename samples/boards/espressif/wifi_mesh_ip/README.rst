.. zephyr:code-sample:: wifi-mesh-ip
   :name: Wi-Fi Mesh IP

   Carry IP over a self-organizing Wi-Fi mesh with a root NAT relay.

Overview
********

This sample carries IP over a self-organizing Wi-Fi mesh so that unicast IPv4
sockets run unmodified on every node (see `Limitations`_ for what the mesh
transport does not carry). It builds on the :zephyr:code-sample:`wifi-mesh`
sample: the nodes form the same self-organizing mesh (combined AP and station
mode, root election by router reachability), and this sample adds an IP layer
on top of the mesh transport.

A small Ethernet interface, provided by the Wi-Fi driver
(``CONFIG_WIFI_ESP32_MESH_IP``), bridges the Zephyr network stack to the mesh:
outbound packets are sent with the mesh send API and inbound mesh frames are
handed to the stack. All IP-level functions are native Zephyr subsystems.

The root serves DHCP to the children on the mesh interface (subnet
192.168.4.0/24) and masquerades their traffic out the station interface with
NAT. A child obtains an address from the root over the mesh (for example
192.168.4.2) and installs a default route toward it. Once the root station has
its own uplink lease from the router, a child reaches the external network
through the root's NAT.

The mesh transport cannot carry the ARP broadcast a child would use to resolve
the root gateway, so the root periodically sends a gratuitous ARP that the mesh
fans out to every child. A freshly joined child therefore may wait up to
``CONFIG_NET_ARP_GRATUITOUS_INTERVAL`` seconds for the next gratuitous ARP
before its uplink resolves the gateway and external traffic starts flowing.

Limitations
===========

The IP-over-mesh transport is unicast IPv4 only. These are properties of the
mesh transport, not bugs, but they shape what runs over it:

* No multicast. The mesh has no multicast primitive, so IPv4 multicast and
  IPv6 neighbor discovery, mDNS and multicast-based service discovery do not
  traverse the mesh.
* Broadcast is emulated as one unicast per known node (O(N) over the routing
  table), used only for the gratuitous-ARP gateway workaround above. It does
  not scale to general broadcast traffic.
* IPv4 only. Addressing, DHCP and the root NAT are IPv4; there is no IPv6 path.
* Reduced MTU with no fragmentation. The mesh interface MTU is smaller than a
  normal Ethernet link, and frames larger than the mesh payload are rejected
  rather than fragmented, so the path MTU must be respected.

In short, unmodified unicast IPv4 (TCP and UDP) works across the mesh;
anything that relies on multicast, broadcast or IPv6 does not.

Requirements
************

* At least two supported ESP32 boards (esp32, esp32s2, esp32s3, esp32c3,
  esp32c5, esp32c6). ESP32-C2 and ESP32-H2 do not provide a mesh library.
* A 2.4 GHz router with internet access. The root associates to it and relays
  child traffic to the external network.

Memory
======

The configuration is sized for a root node, which carries the most: the Wi-Fi
buffer pools for its direct children plus the DHCP server, NAT and routing
state for the sub-network. The same image is flashed to every board because any
node may be elected root.

On a SoC with a smaller RAM budget that does not fit, as on the ESP32 and the
ESP32-S2, trim the mesh fan-out and the Wi-Fi buffer pools (see the board files
under ``boards/``). A node configured that way runs as a leaf child.

Setting ``CONFIG_NET_DHCPV4_SERVER=n`` and ``CONFIG_NET_IPV4_NAT=n`` drops the
root relay entirely and frees the memory it needs. Such a node can never serve
the mesh subnet, so build it only where another node is guaranteed to be root;
it reports an error if it is elected.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/wifi_mesh_ip
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :compact:

Configure the mesh channel and the router credentials through Kconfig
(``CONFIG_WIFI_ESP32_MESH_CHANNEL``, ``CONFIG_WIFI_ESP32_MESH_ROUTER_SSID``,
``CONFIG_WIFI_ESP32_MESH_ROUTER_PSK``). The router credentials default to the
``myssid``/``mypassword`` placeholders and must be set to the router in use;
a node left on the placeholder finds no such network and keeps searching for
a parent. Set the channel to the router's
channel. Flash the same image to every board.

Because root election is driven by router reachability, boards that are all
within range of the router each become their own root; a node joins the mesh
as a child only when it is out of router range. For bench testing with all
boards near the router, build the intended child nodes with
``CONFIG_WIFI_ESP32_MESH_FORCE_CHILD=y`` so they attach to the elected root
instead of each becoming their own root.

Sample Output
*************

The root brings up its uplink and NAT, and a child obtains an address over the
mesh:

.. code-block:: console

   <inf> wifi_mesh_ip: parent connected, layer 1 (root)
   <inf> wifi_mesh_ip: mesh root IP: gateway 192.168.4.1, DHCP + NAT up
   <inf> wifi_mesh_ip: child connected
   <inf> wifi_mesh_ip: root external network reachable
   <inf> wifi_mesh_ip: child got IP 192.168.4.2 over mesh

Once the child has its address, confirm it reaches the external network through
the root's NAT by pinging a public host from the child's shell:

.. code-block:: console

   uart:~$ net ping 1.1.1.1
