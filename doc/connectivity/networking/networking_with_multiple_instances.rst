.. _networking_with_multiple_instances:

Networking with multiple Zephyr instances
#########################################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between multiple
Zephyr instances. The Zephyr instances could be running inside QEMU
or could be native_sim board processes. The Linux host can be used
to route network traffic between these systems.

Prerequisites
*************

On the Linux Host, find the Zephyr `net-tools`_ project, which can either be
found in a Zephyr standard installation under the ``tools/net-tools`` directory
or installed stand alone from its own git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools

Basic Setup
***********

For the steps below, you will need five terminal windows:

* Terminal #1 and #2 are terminal windows with net-tools being the current
  directory (``cd net-tools``)
* Terminal #3, where you setup bridging in Linux host
* Terminal #4 and #5 are your usual Zephyr development terminal,
  with the Zephyr environment initialized.

As there are multiple ways to setup the Zephyr network, the example below uses
``qemu_x86`` board with ``e1000`` Ethernet controller and native_sim board
to simplify the setup instructions. You can use other QEMU boards and drivers
if needed, see :ref:`networking_with_eth_qemu` for details. You can also use
two or more native_sim board Zephyr instances and connect them together.


Step 1 - Create configuration files
===================================

Before starting QEMU with network connectivity, a network interfaces for each
Zephyr instance should be created in the host system. The default setup for
creating network interface cannot be used here as that is for connecting one
Zephyr instance to Linux host.

For Zephyr instance #1, create file called ``zephyr1.conf`` to ``net-tools``
project, or to some other suitable directory.

.. code-block:: console

   # Configuration file for setting IP addresses for a network interface.
   INTERFACE="$1"
   HWADDR="00:00:5e:00:53:11"
   IPV6_ADDR_1="2001:db8:100::2"
   IPV6_ROUTE_1="2001:db8:100::/64"
   IPV4_ADDR_1="198.51.100.2/24"
   IPV4_ROUTE_1="198.51.100.0/24"
   ip link set dev $INTERFACE up
   ip link set dev $INTERFACE address $HWADDR
   ip -6 address add $IPV6_ADDR_1 dev $INTERFACE nodad
   ip -6 route add $IPV6_ROUTE_1 dev $INTERFACE
   ip address add $IPV4_ADDR_1 dev $INTERFACE
   ip route add $IPV4_ROUTE_1 dev $INTERFACE > /dev/null 2>&1

For Zephyr instance #2, create file called ``zephyr2.conf`` to ``net-tools``
project, or to some other suitable directory.

.. code-block:: console

   # Configuration file for setting IP addresses for a network interface.
   INTERFACE="$1"
   HWADDR="00:00:5e:00:53:22"
   IPV6_ADDR_1="2001:db8:200::2"
   IPV6_ROUTE_1="2001:db8:200::/64"
   IPV4_ADDR_1="203.0.113.2/24"
   IPV4_ROUTE_1="203.0.113.0/24"
   ip link set dev $INTERFACE up
   ip link set dev $INTERFACE address $HWADDR
   ip -6 address add $IPV6_ADDR_1 dev $INTERFACE nodad
   ip -6 route add $IPV6_ROUTE_1 dev $INTERFACE
   ip address add $IPV4_ADDR_1 dev $INTERFACE
   ip route add $IPV4_ROUTE_1 dev $INTERFACE > /dev/null 2>&1


Step 2 - Create Ethernet interfaces
===================================

The following ``net-setup.sh`` commands should be typed in net-tools
directory (``cd net-tools``).

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh -c zephyr1.conf -i zeth.1

In terminal #2, type:

.. code-block:: console

   ./net-setup.sh -c zephyr2.conf -i zeth.2


Step 3 - Setup network bridging
===============================

In terminal #3, type:

.. code-block:: console

   sudo brctl addbr zeth-br
   sudo brctl addif zeth-br zeth.1
   sudo brctl addif zeth-br zeth.2
   sudo ifconfig zeth-br up


Step 4 - Start Zephyr instances
===============================

In this example we start :zephyr:code-sample:`sockets-echo-server` and
:zephyr:code-sample:`sockets-echo-client` sample applications. You can use other applications
too as needed.

In terminal #4, if you are using QEMU, type this:

.. code-block:: console

   west build -d build/server -b qemu_x86 -t run \
      samples/net/sockets/echo_server -- \
      -DEXTRA_CONF_FILE=overlay-e1000.conf \
      -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"198.51.100.1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV4_ADDR=\"203.0.113.1\" \
      -DCONFIG_NET_CONFIG_MY_IPV6_ADDR=\"2001:db8:100::1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV6_ADDR=\"2001:db8:200::1\" \
      -DCONFIG_NET_CONFIG_MY_IPV4_GW=\"203.0.113.1\" \
      -DCONFIG_ETH_QEMU_IFACE_NAME=\"zeth.1\" \
      -DCONFIG_ETH_QEMU_EXTRA_ARGS=\"mac=00:00:5e:00:53:01\"

or if you want to use native_sim board, type this:

.. code-block:: console

   west build -d build/server -b native_sim -t run \
      samples/net/sockets/echo_server -- \
      -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"198.51.100.1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV4_ADDR=\"203.0.113.1\" \
      -DCONFIG_NET_CONFIG_MY_IPV6_ADDR=\"2001:db8:100::1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV6_ADDR=\"2001:db8:200::1\" \
      -DCONFIG_NET_CONFIG_MY_IPV4_GW=\"203.0.113.1\" \
      -DCONFIG_ETH_NATIVE_POSIX_DRV_NAME=\"zeth.1\" \
      -DCONFIG_ETH_NATIVE_POSIX_MAC_ADDR=\"00:00:5e:00:53:01\" \
      -DCONFIG_ETH_NATIVE_POSIX_RANDOM_MAC=n


In terminal #5, if you are using QEMU, type this:

.. code-block:: console

   west build -d build/client -b qemu_x86 -t run \
      samples/net/sockets/echo_client -- \
      -DEXTRA_CONF_FILE=overlay-e1000.conf \
      -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"203.0.113.1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV4_ADDR=\"198.51.100.1\" \
      -DCONFIG_NET_CONFIG_MY_IPV6_ADDR=\"2001:db8:200::1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV6_ADDR=\"2001:db8:100::1\" \
      -DCONFIG_NET_CONFIG_MY_IPV4_GW=\"198.51.100.1\" \
      -DCONFIG_ETH_QEMU_IFACE_NAME=\"zeth.2\" \
      -DCONFIG_ETH_QEMU_EXTRA_ARGS=\"mac=00:00:5e:00:53:02\"

or if you want to use native_sim board, type this:

.. code-block:: console

   west build -d build/client -b native_sim -t run \
      samples/net/sockets/echo_client -- \
      -DCONFIG_NET_CONFIG_MY_IPV4_ADDR=\"203.0.113.1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV4_ADDR=\"198.51.100.1\" \
      -DCONFIG_NET_CONFIG_MY_IPV6_ADDR=\"2001:db8:200::1\" \
      -DCONFIG_NET_CONFIG_PEER_IPV6_ADDR=\"2001:db8:100::1\" \
      -DCONFIG_NET_CONFIG_MY_IPV4_GW=\"198.51.100.1\" \
      -DCONFIG_ETH_NATIVE_POSIX_DRV_NAME=\"zeth.2\" \
      -DCONFIG_ETH_NATIVE_POSIX_MAC_ADDR=\"00:00:5e:00:53:02\" \
      -DCONFIG_ETH_NATIVE_POSIX_RANDOM_MAC=n


Also if you have firewall enabled in your host, you need to allow traffic
between ``zeth.1``, ``zeth.2`` and ``zeth-br`` interfaces.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
