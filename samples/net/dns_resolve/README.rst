.. _dns-resolve-sample:

DNS Resolve Application
#######################

Overview
********

This application will setup IP address for the device, and then
try to resolve various hostnames according to how the user has
configured the system.

- If IPv4 is enabled, then A record for ``www.zephyrproject.org`` is
  resolved.
- If IPv6 is enabled, then AAAA record for ``www.zephyrproject.org`` is
  resolved.
- If mDNS is enabled, then ``zephyr.local`` name is resolved.

Requirements
************

- :ref:`networking_with_host`

- screen terminal emulator or equivalent.

- For most boards without ethernet, the ENC28J60 Ethernet module is required.

- dnsmasq application. The dnsmasq version used in this sample is:

.. code-block:: console

    dnsmasq -v
    Dnsmasq version 2.76  Copyright (c) 2000-2016 Simon Kelley

Building and Running
********************

Network Configuration
=====================

Open the project configuration file for your platform, for example:
:file:`prj_frdm_k64f.conf` is the configuration file for the
:ref:`frdm_k64f` board.

In this sample application, both static or DHCPv4 IP addresses are supported.
Static IP addresses are specified in the project configuration file,
for example:

.. code-block:: console

	CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8::1"
	CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8::2"

are the IPv6 addresses for the DNS client running Zephyr and the DNS server,
respectively.

DNS server
==========

The dnsmasq tool may be used for testing purposes. Sample dnsmasq start
script can be downloaded from the zephyrproject-rtos/net-tools project area:
https://github.com/zephyrproject-rtos/net-tools

Open a terminal window and type:

.. code-block:: console

    $ cd net-tools
    $ sudo ./dnsmasq.sh

The default project configurations settings for this sample uses the public
Google DNS servers.  In order to use the local dnsmasq server, please edit
the appropriate 'prj.conf' file and update the DNS server addresses.  For
instance, if using the usual IP addresses assigned to testing, update them
to the following values:

.. code-block:: console

    CONFIG_DNS_SERVER1="192.0.2.2:5353"
    CONFIG_DNS_SERVER2="[2001:db8::2]:5353"

.. note::
    DNS uses port 53 by default, but the dnsmasq.conf file provided by
    net-tools uses port 5353 to allow executing the daemon without
    superuser privileges.

If dnsmasq fails to start with an error like this:

.. code-block:: console

    dnsmasq: failed to create listening socket for port 5353: Address already in use

Open a terminal window and type:

.. code-block:: console

    $ killall -s KILL dnsmasq

Try to launch the dnsmasq application again.

For testing mDNS, use Avahi script in net-tools project:

.. code-block:: console

    $ cd net-tools
    $ ./avahi-daemon.sh


LLMNR Responder
===============

If you want Zephyr to respond to a LLMNR DNS request that Windows host is
sending, then following config options could be set:

.. code-block:: console

    CONFIG_NET_HOSTNAME_ENABLE=y
    CONFIG_NET_HOSTNAME="zephyr-device"
    CONFIG_DNS_RESOLVER=y
    CONFIG_LLMNR_RESPONDER=y

A Zephyr host needs a hostname assigned to it so that it can respond to a DNS
query. Note that the hostname should not have any dots in it.


QEMU x86
========

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.


FRDM K64F
=========

Open a terminal window and type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dns_resolve
   :board: frdm_k64f
   :goals: build flash
   :compact:

See :ref:`Freedom-K64F board documentation <frdm_k64f>` for more information
about this board.

Open a terminal window and type:

.. code-block:: console

    $ screen /dev/ttyACM0 115200


Use 'dmesg' to find the right USB device.

Once the binary is loaded into the FRDM board, press the RESET button.

