.. zephyr:code-sample:: dhcpv6-pd
   :name: DHCPv6 prefix delegation
   :relevant-api: dhcpv6 dhcpv6_server

   Obtain and delegate an IPv6 prefix using DHCPv6 (RFC 8415).

Overview
********

This sample demonstrates IPv6 prefix delegation as specified in :rfc:`8415`.
It can be built in two roles:

* **Requesting router** (default): a DHCPv6 client requests an IPv6 address
  (IA_NA) and a delegated prefix (IA_PD) on its upstream (WAN) interface. Each
  downstream (LAN) interface is then assigned a distinct ``/64`` carved from the
  delegated prefix and advertised via Router Advertisements, so that hosts on
  the downstream links can auto-configure addresses (SLAAC).

* **Delegating router / server** (``overlay-server.conf``): a DHCPv6 server
  hands out addresses and delegated prefixes from configured pools.

Network interfaces
******************

Prefix delegation needs at least two links, so the requesting-router build
brings up two network interfaces. On native_sim board these are two TAP interfaces:

* ``zeth0`` -- upstream (WAN), runs the DHCPv6 client (interface index 1),
* ``zeth1`` -- downstream (LAN), where the delegated prefix is advertised
  (interface index 2).

The number of downstream links is bounded by
:kconfig:option:`CONFIG_NET_DHCPV6_MAX_DOWNSTREAM`; raise it (together with
:kconfig:option:`CONFIG_ETH_NATIVE_TAP_INTERFACE_COUNT` and
:kconfig:option:`CONFIG_NET_IF_MAX_IPV6_COUNT`) to delegate to more links. The
server build uses a single interface (``zeth0``).

To give the TAP interfaces host-side connectivity, set them up with the
``net-setup.sh`` script from the
`net-tools <https://github.com/zephyrproject-rtos/net-tools>`_ project before
running, e.g. one instance as the server and another as the requesting
router. See ``README-dhcpv6-pd.md`` file in net-tools for details.

Building and Running
********************

Requesting router (client):

.. code-block:: console

   west build -b native_sim samples/net/dhcpv6_pd
   west build -t run

Delegating router (server):

.. code-block:: console

   west build -b native_sim samples/net/dhcpv6_pd -- \
       -DEXTRA_CONF_FILE=overlay-server.conf
   west build -t run

Requirements
************

The requesting-router downstream advertisement requires
:kconfig:option:`CONFIG_NET_IPV6_RA` and, for forwarding between the WAN and
LAN interfaces, :kconfig:option:`CONFIG_NET_IPV6_FORWARDING`.
