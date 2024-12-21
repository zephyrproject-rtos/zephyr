.. _networking_with_native_sim_eth_bridge:

Ethernet bridge with native_sim board
#####################################

.. contents::
    :local:
    :depth: 2

This document describes how to set up a bridged Ethernet network between a (Linux) host
and a Zephyr application running in a :ref:`native_sim <native_sim>` board.

This setup is useful when testing the Ethernet bridging feature that can be enabled with
:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE` Kconfig option. In this setup, the net-tools
configuration creates two host network interfaces ``zeth0`` and ``zeth1`` and connects them
to Zephyr's :ref:`native_sim <native_sim>` application.

First create the host interfaces. In this example two interfaces are created.

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   ./net-setup.sh -c zeth-multiface.conf -i zeth0 -t 2

The ``-c`` tells which configuration file to use, where ``zeth-multiface.conf`` is tailored
for generating multiple network interfaces in the host.
The ``-i`` option tells what is the first host interface name. The ``-t`` tells how
many network interfaces to create.

Example output of the host interfaces:

.. code-block:: console

   zeth0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
          inet 192.0.2.2  netmask 255.255.255.255  broadcast 0.0.0.0
          inet6 2001:db8::2  prefixlen 128  scopeid 0x0<global>
          inet6 fe80::200:5eff:fe00:5300  prefixlen 64  scopeid 0x20<link>
          ether 00:00:5e:00:53:00  txqueuelen 1000  (Ethernet)
          RX packets 33  bytes 2408 (2.4 KB)
          RX errors 0  dropped 0  overruns 0  frame 0
          TX packets 49  bytes 4092 (4.0 KB)
          TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   zeth1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
          inet 198.51.100.1  netmask 255.255.255.255  broadcast 0.0.0.0
          inet6 fe80::200:5eff:fe00:5301  prefixlen 64  scopeid 0x20<link>
          inet6 2001:db8:2::1  prefixlen 128  scopeid 0x0<global>
          ether 00:00:5e:00:53:01  txqueuelen 1000  (Ethernet)
          RX packets 21  bytes 1340 (1.3 KB)
          RX errors 0  dropped 0  overruns 0  frame 0
          TX packets 45  bytes 3916 (3.9 KB)
          TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

Then create a sample and enable Ethernet bridging support. In this example we create
:zephyr:code-sample:`sockets-echo-server` sample application.

.. code-block:: console

   west build -p -b native_sim -d ../build/echo-server \
      samples/net/sockets/echo_server -- \
      -DCONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD="\"gnome-terminal -- screen %s\"" \
      -DCONFIG_NET_ETHERNET_BRIDGE=y \
      -DCONFIG_NET_ETHERNET_BRIDGE_SHELL=y \
      -DCONFIG_ETH_NATIVE_POSIX_INTERFACE_COUNT=2 \
      -DCONFIG_NET_IF_MAX_IPV6_COUNT=2 \
      -DCONFIG_NET_IF_MAX_IPV4_COUNT=2
   ../build/echo-server/zephyr/zephyr.exe -attach_uart

This will create and run :zephyr:code-sample:`sockets-echo-server` with bridging enabled but
not yet configured. To configure the bridging, you either need to use the bridge shell or call the
bridging API directly from the application. We setup the bridging using the bridge shell like
this:

.. code-block:: console

   net bridge addif 1 3 2
   net iface up 1

In the above example, the bridge interface index is 1, and interfaces 2 and 3 are
Ethernet interfaces which are linked to interfaces ``zeth0`` and ``zeth1`` in the host side.

The network interfaces look like this in Zephyr's side:

.. code-block:: console

   net iface
   Hostname: zephyr

   Interface bridge0 (0x8090ebc) (Virtual) [1]
   ==================================
   Virtual name : <enabled>
   No attached network interface.
   Link addr : 3B:DB:31:0F:CC:B6
   MTU       : 1500
   Flags     : NO_AUTO_START
   Device    : BRIDGE_0 (0x8088354)
   Promiscuous mode : disabled
   IPv6 not enabled for this interface.
   IPv4 not enabled for this interface.

   Interface eth0 (0x8090fcc) (Ethernet) [2]
   ===================================
   Link addr : 02:00:5E:00:53:D2
   MTU       : 1500
   Flags     : AUTO_START,IPv4,IPv6
   Device    : zeth0 (0x808837c)
   Promiscuous mode : disabled
   Ethernet capabilities supported:
           TXTIME
           Promiscuous mode
   Ethernet PHY device: <none> (0)
   IPv6 unicast addresses (max 3):
           fe80::5eff:fe00:53d2 autoconf preferred infinite
           2001:db8::1 manual preferred infinite
   IPv6 multicast addresses (max 4):
           ff02::1
           ff02::1:ff00:53d2
           ff02::1:ff00:1
   IPv6 prefixes (max 2):
           <none>
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 18476
   IPv6 retransmit timer    : 0
   IPv4 unicast addresses (max 1):
           192.0.2.1/255.255.255.0 manual preferred infinite
   IPv4 multicast addresses (max 2):
           224.0.0.1
   IPv4 gateway : 0.0.0.0

   Interface eth1 (0x80910dc) (Ethernet) [3]
   ===================================
   Link addr : 02:00:5E:00:53:87
   MTU       : 1500
   Flags     : AUTO_START,IPv4,IPv6
   Device    : zeth1 (0x8088368)
   Promiscuous mode : disabled
   Ethernet capabilities supported:
           TXTIME
           Promiscuous mode
   Ethernet PHY device: <none> (0)
   IPv6 unicast addresses (max 3):
           fe80::5eff:fe00:5387 autoconf preferred infinite
   IPv6 multicast addresses (max 4):
           ff02::1
           ff02::1:ff00:5387
   IPv6 prefixes (max 2):
           <none>
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 25158
   IPv6 retransmit timer    : 0
   IPv4 unicast addresses (max 1):
           <none>
   IPv4 multicast addresses (max 2):
           224.0.0.1
   IPv4 gateway : 0.0.0.0

The ``net bridge`` command will show the current status of the bridging:

.. code-block:: console

   net bridge
   Bridge Status   Config   Interfaces
   1      up       ok       2 3

The ``addif`` command adds Ethernet interfaces 2 and 3 to the bridge interface 1.
After the ``addif`` command, the bridging is still disabled because the bridge interface
is not up by default. The ``net iface up`` command will turn on bridging.

If you have wireshark running in host side and monitoring ``zeth0`` and ``zeth1``,
you should see the same network traffic in both host interfaces.

Note that interface index numbers are not fixed, the bridge and Ethernet interface index
values might be different in your setup.

The bridging can be disabled by taking the bridge interface down, and the Ethernet interfaces
can be removed from the bridge using ``delif`` command.

.. code-block:: console

   net iface down 1
   net bridge delif 1 2 3
