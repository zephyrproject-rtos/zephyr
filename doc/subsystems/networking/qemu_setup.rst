.. _networking_with_qemu:

Networking with QEMU
####################

This page describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in a QEMU virtual machine (built for Zephyr
targets such as qemu_x86 and qemu_cortex_m3).

In this example, the :ref:`echo-server-sample` sample application from
the Zephyr source distribution is run in QEMU. The QEMU instance is
connected to a Linux host using a serial port, and SLIP is used to
transfer data between the Zephyr application and Linux (over a chain of
virtual connections).

Prerequisites
*************

On the Linux Host, fetch the Zephyr ``net-tools`` project, which is located
in a separate Git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools
   cd net-tools
   make

.. note::

   If you get an error about AX_CHECK_COMPILE_FLAG, install package
   ``autoconf-archive`` package on Debian/Ubuntu.

Basic Setup
***********

For the steps below, you will need at least 4 terminal windows:

* Terminal #1 is your usual Zephyr development terminal, with the Zephyr environment
  initialized.
* Terminals #2, #3, and #4 are terminal windows with net-tools being the current
  directory (``cd net-tools``)

Step 1 - Create helper socket
=============================

Before starting QEMU with network emulation, a Unix socket for the emulation
should be created.

In terminal #2, type:

.. code-block:: console

   ./loop-socat.sh

Step 2 - Start TAP device routing daemon
========================================

In terminal #3, type:


.. code-block:: console

   sudo ./loop-slip-tap.sh

For applications requiring DNS, you may need to restart the host's DNS server
at this point, as described in :ref:`networking_internet`.

Step 3 - Start app in QEMU
==========================

Build and start the ``echo_server`` sample application.

In terminal #1, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_server
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

If you see an error from QEMU about unix:/tmp/slip.sock, it means you missed Step 1
above.

Step 4 - Run apps on host
=========================

Now in terminal #4, you can run various tools to communicate with the
application running in QEMU.

You can start with pings:

.. code-block:: console

   ping 192.0.2.1
   ping6 2001:db8::1

You can use the netcat ("nc") utility, connecting using UDP:

.. code-block:: console

   echo foobar | nc -6 -u 2001:db8::1 4242
   foobar

.. code-block:: console

   echo foobar | nc -u 192.0.2.1 4242
   foobar

If echo_server is compiled with TCP support (now enabled by default for
the echo_server sample, CONFIG_NET_TCP=y):

.. code-block:: console

   echo foobar | nc -6 -q2 2001:db8::1 4242
   foobar

.. note::

   Use Ctrl+C to exit.

You can also use the telnet command to achieve the above.

Step 5 - Stop supporting daemons
================================

When you are finished with network testing using QEMU, you should stop
any daemons or helpers started in the initial steps, to avoid possible
networking or routing problems such as address conflicts in local
network interfaces. For example, stop them if you switch from testing
networking with QEMU to using real hardware, or to return your host
laptop to normal WiFi use.

To stop the daemons, press Ctrl+C in the corresponding terminal windows
(you need to stop both ``loop-slip-tap.sh`` and ``loop-socat.sh``).


.. _networking_internet:

Setting up Zephyr and NAT/masquerading on host to access Internet
*****************************************************************

To access the internet from a Zephyr application, some additional
setup on the host may be required. This setup is common for both
application running in QEMU and on real hardware, assuming that
a development board is connected to the development host. If a
board is connected to a dedicated router, it should not be needed.

To access the internet from a Zephyr application using IPv4,
a gateway should be set via DHCP or configured manually.
For applications using the "Settings" part of :ref:`net_app_api`
facility (with the config option :option:`CONFIG_NET_CONFIG_SETTINGS` enabled),
set the :option:`CONFIG_NET_CONFIG_MY_IPV4_GW` option to the IP address
of the gateway. For apps not using the :ref:`net_app_api` facility, set up the
gateway by calling the :c:func:`net_if_ipv4_set_gw` at runtime.

To access the internet from a custom application running in QEMU, NAT
(masquerading) should be set up for QEMU's source address. Assuming 192.0.2.1 is
used, the following command should be run as root:

.. code-block:: console

   iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1

Additionally, IPv4 forwarding should be enabled on the host, and you may need to
check that other firewall (iptables) rules don't interfere with masquerading.
To enable IPv4 forwarding the following command should be run as root:

.. code-block:: console

   sysctl -w net.ipv4.ip_forward=1

Some applications may also require a DNS server. A number of Zephyr-provided
samples assume by default that the DNS server is available on the host
(IP 192.0.2.2), which, in modern Linux distributions, usually runs at least
a DNS proxy. When running with QEMU, it may be required to restart the host's
DNS, so it can serve requests on the newly created TAP interface. For example,
on Debian-based systems:

.. code-block:: console

   service dnsmasq restart

An alternative to relying on the host's DNS server is to use one in the
network. For example, 8.8.8.8 is a publicly available DNS server. You can
configure it using :option:`CONFIG_DNS_SERVER1` option.


Network connection between two QEMU VMs
***************************************

Unlike the VM-to-Host setup described above, VM-to-VM setup is
automatic. For sample
applications that support this mode (such as the echo_server and echo_client
samples), you will need two terminal windows, set up for Zephyr development.

Terminal #1:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_server
   :host-os: unix
   :board: qemu_x86
   :goals: build
   :build-args: server
   :compact:

This will start QEMU, waiting for a connection from a client QEMU.

Terminal #2:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_client
   :host-os: unix
   :board: qemu_x86
   :goals: build
   :build-args: client
   :compact:

This will start a second QEMU instance, where you should see logging of data sent and
received in both.

Running multiple QEMU VMs of the same sample
********************************************

If you find yourself wanting to run multiple instances of the same Zephyr
sample application, which do not need to talk to each other, use the
``QEMU_INSTANCE`` argument.

Start ``socat`` and ``tunslip6`` manually (instead of using the
``loop-xxx.sh`` scripts) for as many instances as you want. Use the
following as a guide, replacing MAIN or OTHER.

Terminal #1:
============

.. code-block:: console

   socat PTY,link=/tmp/slip.devMAIN UNIX-LISTEN:/tmp/slip.sockMAIN
   $ZEPHYR_BASE/../net-tools/tunslip6 -t tapMAIN -T -s /tmp/slip.devMAIN \
        2001:db8::1/64
   # Now run Zephyr
   make run QEMU_INSTANCE=MAIN

Terminal #2:
============

.. code-block:: console

   socat PTY,link=/tmp/slip.devOTHER UNIX-LISTEN:/tmp/slip.sockOTHER
   $ZEPHYR_BASE/../net-tools/tunslip6 -t tapOTHER -T -s /tmp/slip.devOTHER \
        2001:db8::1/64
   make run QEMU_INSTANCE=OTHER
