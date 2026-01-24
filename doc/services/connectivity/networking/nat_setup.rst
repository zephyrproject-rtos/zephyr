.. _networking_internet:

Setting up NAT/masquerading on host to access Internet
######################################################

.. contents::
    :local:
    :depth: 2

To access the internet from a Zephyr application, some additional
setup on the host may be required. This setup is common for applications
running in QEMU, in a :zephyr:board:`native_sim <native_sim>` board and on real
hardware, assuming that a development board is connected to the development
host. If a board is connected to a dedicated router, it should not be needed.

Gateway
*******

To access the internet from a Zephyr application using IPv4,
a gateway should be set via DHCP or configured manually.
For applications using the "Settings" facility (with the config option
:kconfig:option:`CONFIG_NET_CONFIG_SETTINGS` enabled),
set the :kconfig:option:`CONFIG_NET_CONFIG_MY_IPV4_GW` option to the IP address
of the gateway. For apps not using the "Settings" facility, set up the
gateway by calling the :c:func:`net_if_ipv4_set_gw` at runtime.
For example: ``CONFIG_NET_CONFIG_MY_IPV4_GW="192.0.2.2"``

NAT/masquerading
****************

NAT (masquerading) should be set up on the host for the source address of the
Zephyr application. The examples below assume that the application uses the
address ``192.0.2.1`` and that the host network interface connected to it is
``zeth``.

Using net-tools
===============

When the host network interface is created with the ``net-setup.sh`` script of
the `net-tools`_ project, as it is done in :ref:`networking_with_native_sim`
and :ref:`networking_with_eth_qemu`, the ``nat.conf`` configuration file can be
used instead of the default one. It sets up masquerading and IPv4 forwarding,
and starts a DNS server for the interface. Navigate to the `net-tools`_
directory and run the following command as root:

.. code-block:: console

   ./net-setup.sh start --config nat.conf

To remove the interface and stop NAT, run the following command as root:

.. code-block:: console

   ./net-setup.sh stop --config nat.conf

.. warning::

   The ``nat.conf`` configuration changes host-wide settings and does not
   restore their previous values:

   * Starting sets the policy of the host's ``FORWARD`` chain to ``ACCEPT``.
     Stopping does not restore the previous policy.
   * Stopping disables IPv4 forwarding (``net.ipv4.ip_forward=0``) even if it
     was enabled before starting. This can break the networking of containers
     and virtual machines running on the host, for example Docker.

Manual setup
============

In other cases the following commands should be run as root:

.. code-block:: console

   iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1/24
   iptables -I FORWARD 1 -i zeth -j ACCEPT
   iptables -I FORWARD 1 -o zeth -m state --state RELATED,ESTABLISHED -j ACCEPT

Additionally, IPv4 forwarding should be enabled on the host, and you may need to
check that other firewall (iptables) rules don't interfere with masquerading.
To enable IPv4 forwarding the following command should be run as root:

.. code-block:: console

   sysctl -w net.ipv4.ip_forward=1

DNS
***

Some applications may also require a DNS server. A number of Zephyr-provided
samples assume by default that the DNS server is available on the host
(IP ``192.0.2.2``), which, in modern Linux distributions, usually runs at least
a DNS proxy.

When the ``nat.conf`` configuration of `net-tools`_ is used, a DNS server is
already running on the interface and nothing else needs to be done.

With the manual setup, it may be required to restart the host's DNS, so it can
serve requests on the newly created TAP interface. For example, on Debian-based
systems:

.. code-block:: console

   service dnsmasq restart

An alternative to relying on the host's DNS server is to use one in the
network. For example, ``8.8.8.8`` is a publicly available DNS server. You can
configure it using :kconfig:option:`CONFIG_DNS_SERVER1` option.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
