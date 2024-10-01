.. zephyr:code-sample:: vlan
   :name: Virtual LAN
   :relevant-api: vlan_api net_l2 net_if

   Setup two virtual LAN networks and use net-shell to view the networks' settings.

Overview
********

The VLAN sample application for Zephyr will setup two virtual LAN networks.
The application sample enables net-shell and allows users to view VLAN settings.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/vlan`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

A good way to run this VLAN application is with QEMU as described in
:ref:`networking_with_eth_qemu`. You can use :file:`zeth-vlan.conf` configuration
file when running :file:`net-setup.sh` script in Linux like this:

.. code-block:: console

    ./net-setup.sh -c zeth-vlan.conf

Note that VLAN is only supported for boards that have an ethernet port or
that support USB networking.

Follow these steps to build the VLAN sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/vlan
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

The default configuration file :zephyr_file:`samples/net/vlan/prj.conf` creates
two virtual LAN networks with these settings:

- VLAN tag 100: IPv4 198.51.100.1 and IPv6 2001:db8:100::1
- VLAN tag 200: IPv4 203.0.113.1 and IPv6 2001:db8:200::1

Setting up Linux Host
=====================

The :zephyr_file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that
can be executed on the Linux host. It creates two VLAN interfaces ``vlan.100``
and ``vlan.200`` on the Linux host and creates routes to Zephyr.

If everything is configured correctly, you will be able to successfully execute
the following commands on the Linux host.

.. code-block:: console

    ping -c 1 2001:db8:100::1
    ping -c 1 198.51.100.1
    ping -c 1 2001:db8:200::1
    ping -c 1 203.0.113.1

The network packets to ``2001:db8:100::1`` or ``198.51.100.1`` will have VLAN
tag 100 set to them. The vlan tag 200 will be set to network packets to
``2001:db8:200::1`` or ``203.0.113.1``.
