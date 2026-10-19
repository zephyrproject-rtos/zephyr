.. _ethernet_bridge_interface:

Ethernet Bridge
###############

.. contents::
    :local:
    :depth: 2

Overview
********

An Ethernet bridge connects several Ethernet interfaces, its ports, into one LAN: a frame received
on a port is sent out of the other ports. Enable it with
:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE`.

Each bridge is a virtual network interface. Add ports to it with :c:func:`eth_bridge_iface_add`,
or with ``net bridge addif`` when :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_SHELL` is set. A
port is put into promiscuous mode, so its driver must support it, unless the port is a DSA switch
user port. The bridge forwards frames once it has two ports, and its interface can then be brought
up. Give the bridge interface an IP address if the device itself is reached through the bridge.

:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_COUNT` sets the number of bridges, and
:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT` the number of ports of each.
The bridge MAC address is random by default, and new on every boot.
:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_UNIQUE_MAC` derives a stable one from the device ID,
and :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_PREFIX_MAC` builds one from a configured prefix.

The :zephyr:code-sample:`bridge` sample bridges two interfaces, and
:ref:`networking_with_native_sim_eth_bridge` shows how to try it against a Linux host.

Forwarding database
*******************

With :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB`, the bridge sends a frame to a known
destination out of one port instead of all of them. The forwarding database (FDB) maps MAC
addresses to ports:

* Static entries are added with :c:func:`eth_bridge_fdb_add` or ``net bridge fdb add``.
* With :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING`, the default, the bridge learns
  the port of each source address it receives. A learned entry ages out after
  :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_AGEING_TIME` seconds without a frame from its
  address.

:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_MAX_ENTRIES` bounds the table. ``net bridge fdb
show`` lists it.

Spanning tree
*************

Ports of bridges cabled into a loop would send a broadcast round the loop for ever. With
:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_STP`, the bridge runs the Rapid Spanning Tree Protocol
(RSTP) of IEEE Std 802.1Q-2014 Clause 13. The bridges elect a root bridge and block the ports that
would close a loop, and on point-to-point links a blocked port takes over in less than a second
when a link fails. On a
port where a bridge that only speaks the older STP is attached, the protocol falls back to STP
BPDUs and timers. The support is experimental.

The protocol is off by default. Start it on a bridge with :c:func:`eth_bridge_stp_enable`. While
it starts, every port discards frames until the protocol has decided its role. When it is
stopped, every port forwards again.

Settings
========

The defaults are the values IEEE Std 802.1Q-2014 recommends, from the table or clause given:

.. list-table::
   :header-rows: 1

   * - Setting
     - Default
     - IEEE Std 802.1Q-2014
     - Function
   * - Bridge Priority
     - 32768
     - Table 13-3
     - :c:func:`eth_bridge_stp_set_bridge_priority`
   * - Max Age, Forward Delay
     - 20 s, 15 s
     - Table 13-5
     - :c:func:`eth_bridge_stp_set_times`
   * - Transmit Hold Count
     - 6
     - Table 13-5
     - :c:func:`eth_bridge_stp_set_tx_hold_count`
   * - Force Protocol Version
     - RSTP
     - 13.7.2
     - :c:func:`eth_bridge_stp_set_version`
   * - Port Priority
     - 128
     - Table 13-3
     - :c:func:`eth_bridge_stp_port_set_priority`
   * - Port Path Cost
     - from the link speed, 200000 at 100 Mb/s and 20000 at 1 Gb/s
     - Table 13-4
     - :c:func:`eth_bridge_stp_port_set_path_cost`
   * - Edge Port
     - detected when no BPDU arrives
     - 13.27.1, 13.27.18
     - :c:func:`eth_bridge_stp_port_set_edge`
   * - Point-to-point link
     - from the duplex of the link
     - adminPointToPointMAC Auto, IEEE Std 802.1AC
     - :c:func:`eth_bridge_stp_port_set_p2p`

The Hello Time is fixed at 2 seconds (Table 13-5). :c:func:`eth_bridge_stp_port_mcheck` makes a
port check whether an STP bridge is still attached, and send RST BPDUs again if not.

The Bridge Identifier is made of the Bridge Priority and the MAC address of the bridge interface.
Use a stable bridge MAC address, so that the identifier and the elected root do not change on
every boot.

Status and events
=================

:c:func:`eth_bridge_stp_get_status` returns the root bridge, the root port and the topology change
count of a bridge, and :c:func:`eth_bridge_stp_port_get_status` the role and state of a port.

The bridge raises net_mgmt events, with the information in a struct:

.. list-table::
   :header-rows: 1

   * - Event
     - Interface
     - Raised when
   * - ``NET_EVENT_ETHERNET_BRIDGE_PORT_STATE``
     - the port
     - its role, learning or forwarding state changes
       (:c:struct:`eth_bridge_stp_port_event`)
   * - ``NET_EVENT_ETHERNET_BRIDGE_NEW_ROOT``
     - the bridge
     - the root bridge changes (:c:struct:`eth_bridge_stp_root_event`)
   * - ``NET_EVENT_ETHERNET_BRIDGE_TOPOLOGY_CHANGE``
     - the bridge
     - a topology change starts, and the learned FDB entries are flushed
       (:c:struct:`eth_bridge_stp_topology_event`)

The events are raised from the system work queue without the bridge locked, so a callback can
call the bridge functions. A change of the tree raises several events one after the other, so
raise :kconfig:option:`CONFIG_NET_MGMT_EVENT_QUEUE_SIZE` from its default if events are lost.
``net events on`` prints the events when :kconfig:option:`CONFIG_NET_MGMT_EVENT_MONITOR` is set.

Shell
=====

``net bridge stp`` shows and sets the protocol, and takes interface indexes, as ``net iface``
lists them. Without options it shows the bridge or the port, and ``-h`` lists the options:

.. code-block:: console

   uart:~$ net bridge stp 1 --priority 4096 --enable on
   uart:~$ net bridge stp 1
   uart:~$ net bridge stp 1 port 2 --cost 20000 --link p2p
   uart:~$ net bridge stp 1 port 2 --mcheck

Limitations
===========

* The protocol blocks a port in software. A device that forwards frames between its ports in
  hardware cannot be blocked that way, so the protocol does not start on a bridge with such a
  port: DSA switch user ports, and devices that report the
  :c:enumerator:`ETHERNET_HW_PORT_FORWARDING` capability, such as the ADIN2111.
* A bridge with a single port does not receive frames, BPDUs included.
* BPDUs are not forwarded, also while the protocol is off.
* MSTP and the BPDU and root guards are not supported.

Testing against Linux
=====================

A Linux bridge with STP on runs the kernel's STP, or RSTP when the ``mstpd`` daemon takes it over,
so Zephyr and Linux bridges can be cabled into a ring. The kernel bridge uses the short path costs
of IEEE Std 802.1D-1998, 19 at 100 Mb/s, and accepts costs up to 65535 only, while RSTP uses
200000. Set small port costs on both sides, so that the tree is the one intended:

.. code-block:: console

   # ip link add brA type bridge stp_state 1
   # ip link set eth1 master brA
   # bridge link set dev eth1 cost 10

.. code-block:: console

   uart:~$ net bridge stp 1 port 2 --cost 10

``net bridge stp`` on the Zephyr side and ``bridge link``, or ``mstpctl showport`` with mstpd, on
the Linux side then show one blocked port in the ring.

API Reference
*************

.. doxygengroup:: eth_bridge

.. doxygengroup:: eth_bridge_fdb

.. doxygengroup:: eth_bridge_stp
