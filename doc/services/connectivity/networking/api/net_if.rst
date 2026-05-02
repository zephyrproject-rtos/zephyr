.. _net_if_interface:

Network Interface
#################

.. contents::
    :local:
    :depth: 2

Overview
********

The network interface is a nexus that ties the network device drivers
and the upper part of the network stack together. All the sent and received
data is transferred via a network interface. The network interfaces cannot be
created at runtime. A special linker section will contain information about them
and that section is populated at linking time.

Network interfaces are created by ``NET_DEVICE_INIT()`` macro.
For Ethernet network, a macro called ``ETH_NET_DEVICE_INIT()`` should be used
instead as it will create VLAN interfaces automatically if
:kconfig:option:`CONFIG_NET_VLAN` is enabled. These macros are typically used in
network device driver source code.

The network interface can be turned ON by calling ``net_if_up()`` and OFF
by calling ``net_if_down()``. When the device is powered ON, the network
interface is also turned ON by default.

The network interfaces can be referenced either by a ``struct net_if *``
pointer or by a network interface index. The network interface can be
resolved from its index by calling ``net_if_get_by_index()`` and from interface
pointer by calling ``net_if_get_by_iface()``.

.. _net_if_interface_ip_management:

The IP address for network devices must be set for them to be connectable.
In a typical dynamic network environment, IP addresses are set automatically
by DHCPv4, for example. If needed though, the application can set a device's
IP address manually.  See the API documentation below for functions such as
``net_if_ipv4_addr_add()`` that do that.

The ``net_if_get_default()`` returns a *default* network interface. What
this default interface means can be configured via options like
:kconfig:option:`CONFIG_NET_DEFAULT_IF_FIRST` and
:kconfig:option:`CONFIG_NET_DEFAULT_IF_ETHERNET`.
See Kconfig file :zephyr_file:`subsys/net/ip/Kconfig` what options are available for
selecting the default network interface.

The transmitted and received network packets can be classified via a network
packet priority. This is typically done in Ethernet networks when virtual LANs
(VLANs) are used. Higher priority packets can be sent or received earlier than
lower priority packets. The traffic class setup can be configured by
:kconfig:option:`CONFIG_NET_TC_TX_COUNT` and :kconfig:option:`CONFIG_NET_TC_RX_COUNT` options.

If the :kconfig:option:`CONFIG_NET_PROMISCUOUS_MODE` is enabled and if the underlying
network technology supports promiscuous mode, then it is possible to receive
all the network packets that the network device driver is able to receive.
See :ref:`promiscuous_interface` API for more details.

.. _net_if_interface_state_management:

Network interface state management
**********************************

Zephyr distinguishes between two interface states: administrative state and
operational state, as described in RFC 2863. The administrative state indicate
whether an interface is turned ON or OFF. This state is represented by
:c:enumerator:`NET_IF_UP` flag and is controlled by the application. It can be
changed by calling :c:func:`net_if_up` or :c:func:`net_if_down` functions.
Network drivers or L2 implementations should not change administrative state on
their own.

Bringing an interface up however not always means that the interface is ready to
transmit packets. Because of that, operational state, which represents the
internal interface status, was implemented. The operational state is updated
whenever one of the following conditions take place:

  * The interface is brought up/down by the application (administrative state
    changes).
  * The interface is notified by the driver/L2 that PHY status has changed.
  * The interface is notified by the driver/L2 that it joined/left a network.

The PHY status is represented with :c:enumerator:`NET_IF_LOWER_UP` flag and can
be changed with :c:func:`net_if_carrier_on` and :c:func:`net_if_carrier_off`. By
default, the flag is set on a newly initialized interface. An example of an
event that changes the carrier state is Ethernet cable being plugged in or out.

The network association status is represented with :c:enumerator:`NET_IF_DORMANT`
flag and can be changed with :c:func:`net_if_dormant_on` and
:c:func:`net_if_dormant_off`. By default, the flag is cleared on a newly
initialized interface. An example of an event that changes the dormant state is
a Wi-Fi driver successfully connecting to an access point. In this scenario,
driver should set the dormant state to ON during initialization, and once it
detects that it connected to a Wi-Fi network, the dormant state should be set
to OFF.

The operational state of an interface is updated as follows:

  * ``!net_if_is_admin_up()``

    Interface is in :c:enumerator:`NET_IF_OPER_DOWN`.

  * ``net_if_is_admin_up() && !net_if_is_carrier_ok()``

    Interface is in :c:enumerator:`NET_IF_OPER_DOWN` or
    :c:enumerator:`NET_IF_OPER_LOWERLAYERDOWN` if the interface is stacked
    (virtual).

  * ``net_if_is_admin_up() && net_if_is_carrier_ok() && net_if_is_dormant()``

    Interface is in :c:enumerator:`NET_IF_OPER_DORMANT`.

  * ``net_if_is_admin_up() && net_if_is_carrier_ok() && !net_if_is_dormant()``

    Interface is in :c:enumerator:`NET_IF_OPER_UP`.

Only after an interface enters :c:enumerator:`NET_IF_OPER_UP` state the
:c:enumerator:`NET_IF_RUNNING` flag is set on the interface indicating that the
interface is ready to be used by the application.

API Reference
*************

.. doxygengroup:: net_if
