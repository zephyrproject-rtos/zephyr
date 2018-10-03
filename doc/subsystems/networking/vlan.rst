.. _vlan-support:

Virtual LAN (VLAN) Support
##########################

Overview
********

`Virtual LAN <https://wikipedia.org/wiki/Virtual_LAN>`_ (VLAN) is a
partitioned and isolated computer network at the data link layer
(OSI layer 2). For ethernet network this refers to
`IEEE 802.1Q <https://en.wikipedia.org/wiki/IEEE_802.1Q>`_

In Zephyr, each individual VLAN is modeled as a virtual network interface.
This means that there is an ethernet network interface that corresponds to
a real physical ethernet port in the system. A virtual network interface is
created for each VLAN, and this virtual network interface connects to the
real network interface. This is similar to how Linux implements VLANs. The
*eth0* is the real network interface and *vlan0* is a virtual network interface
that is run on top of *eth0*.

VLAN support must be enabled at compile time by setting option
:option:`CONFIG_NET_VLAN` and also setting option
:option:`CONFIG_NET_VLAN_COUNT` to reflect how many network interfaces there
will be in the system.  For example, if there is one network interface without
VLAN support, and two with VLAN support, the :option:`CONFIG_NET_VLAN_COUNT`
should be set to 3.

Even if VLAN is enabled in a :file:`prj.conf` file, the VLAN needs to be
activated at runtime by the application. The VLAN API provides a
:cpp:func:`net_eth_vlan_enable()` function to do that. The application needs
to give the network interface and desired VLAN tag as a parameter to that
function. The VLAN tagging for a given network interface can be disabled by a
:cpp:func:`net_eth_vlan_disable()` function. The application needs to configure
the VLAN network interface itself, such as setting the IP address, etc.

See also the :ref:`vlan-sample` VLAN sample application for API usage
example. The source code for that sample application can be found at
:file:`samples/net/vlan`.

The net-shell module contains *net vlan add* and *net vlan del* commands
that can be used to enable or disable VLAN tags for a given network interface.

See the `IEEE 802.1Q spec`_ for more information about ethernet VLANs.

.. _IEEE 802.1Q spec: https://ieeexplore.ieee.org/document/6991462/
