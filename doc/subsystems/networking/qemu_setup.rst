.. _networking_with_qemu:

Networking with QEMU
####################

This page describes how to set up a "virtual" networking between a (Linux) host
and a Zephyr application running in a QEMU virtual machine (built for Zephyr
targets like qemu_x86, qemu_cortex_m3, etc.) In this example, the
``echo_server`` sample application from Zephyr source distribution is run in
QEMU. The QEMU instance is connected to Linux host using serial port and SLIP is
used to transfer data between Zephyr and Linux (over a chain of virtual
connections).

Prerequisites
*************

On the Linux Host you need to fetch Zephyr net-tools project, which is located
in a separate git repository:

.. code-block:: console

   $ git clone https://github.com/zephyrproject-rtos/net-tools
   $ cd net-tools
   $ make

.. note::

   If you get error about AX_CHECK_COMPILE_FLAG, install package autoconf-archive
   package on Debian/Ubuntu.

Basic Setup
***********

For the steps below, you will need at least 4 terminal windows:

* Terminal #1 is your usual Zephyr development terminal, with Zephyr environment
  initialized.
* Terminals #2, #3, #4 - fresh terminal windows with net-tools being the current
  directory ("cd net-tools")

Step 1 - Create helper socket
=============================

Before starting QEMU with network emulation, a Unix socket for the emulation
should be created.

In terminal #2, type:

.. code-block:: console

   $ ./loop-socat.sh

Step 2 - Start TAP device routing daemon
========================================

In terminal #3, type:


.. code-block:: console

   $ sudo ./loop-slip-tap.sh


Step 3 - Start app in QEMU
==========================

Build and start the ``echo_server`` sample application.

In terminal #1, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_server
   :board: qemu_x86
   :goals: run
   :compact:

If you see error from QEMU about unix:/tmp/slip.sock, it means you missed Step 1
above.

Step 4 - Run apps on host
=========================

Now in terminal #4, you can run various tools to communicate with the
application running in QEMU.

You can start with pings:

.. code-block:: console

   $ ping 192.0.2.1
   $ ping6 2001:db8::1

For example, using netcat ("nc") utility, connecting using UDP:

.. code-block:: console

   $ echo foobar | nc -6 -u 2001:db8::1 4242
   foobar

.. code-block:: console

   $ echo foobar | nc -u 192.0.2.1 4242
   foobar

If echo_server is compiled with TCP support (now enabled by default for
echo_server sample, CONFIG_NET_TCP=y):

.. code-block:: console

   $ echo foobar | nc -6 -q2 2001:db8::1 4242
   foobar

.. note::

   You will need to Ctrl+C manually.

You can also use the telnet command to achieve the above.

Step 5 - Stop supporting daemons
================================

When you are finished with network testing using QEMU, you should stop
any daemons or helpers started in the initial steps, to avoid possible
networking or routing problems such as address conflicts in local network
interfaces. For example, you definitely need to stop them if you switch
from testing networking with QEMU to using real hardware. For example,
there was a report of an airport WiFi connection not working during
travel due to an address conflict.

To stop the daemons, just press Ctrl+C in the corresponding terminal windows
(you need to stop both ``loop-slip-tap.sh`` and ``loop-socat.sh``).


Setting up Zephyr and NAT/masquerading on QEMU host to access Internet
**********************************************************************

To access the Internet from a Zephyr application using IPv4,
a gateway should be set via DHCP or configured manually.
For applications using the :ref:`net_app_api` facility (with the config option
:option:`CONFIG_NET_APP` enabled),
set the :option:`CONFIG_NET_APP_MY_IPV4_GW` option to the IP address
of the gateway. For apps not using the :ref:`net_app_api` facility, set up the
gateway by calling the :c:func:`net_if_ipv4_set_gw` at runtime.

To access Internet from a custom application running in a QEMU, NAT
(masquerading) should be set up for QEMU's source address. Assuming 192.0.2.1 is
used, the following command should be run as root:

.. code-block:: console

   $ iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1

Additionally, IPv4 forwarding should be enabled on host, and you may need to
check that other firewall (iptables) rules don't interfere with masquerading.

Network connection between two QEMU VMs
***************************************

Unlike VM-Host setup described above, VM-VM setup is automatic - for sample
applications which support such mode such as the echo_server and echo_client
samples, you will need 2 terminal windows, set up for Zephyr development.

Terminal #1:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_server
   :board: qemu_x86
   :goals: build
   :build-args: server
   :compact:

This will start QEMU, waiting for connection from a client QEMU.

Terminal #2:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_client
   :board: qemu_x86
   :goals: build
   :build-args: client
   :compact:

This will start 2nd QEMU instance, and you should see logging of data sent and
received in both.

Running multiple QEMU VMs of the same sample
********************************************

If you find yourself needing to run multiple instances of the same Zephyr
sample application, which do not need to be able to talk to each other, the
``QEMU_INSTANCE`` argument is what you need.

Start socat and tunslip6 manually (avoiding loop-x.sh scripts) for as many
instances as you want. Use the following as a guide, replacing MAIN or OTHER.

Terminal #1:
============

.. code-block:: console

   $ socat PTY,link=/tmp/slip.devMAIN UNIX-LISTEN:/tmp/slip.sockMAIN
   $ $ZEPHYR_BASE/../net-tools/tunslip6 -t tapMAIN -T -s /tmp/slip.devMAIN \
        2001:db8::1/64
   # Now run Zephyr
   $ make run QEMU_INSTANCE=MAIN

Terminal #2:
============

.. code-block:: console

   $ socat PTY,link=/tmp/slip.devOTHER UNIX-LISTEN:/tmp/slip.sockOTHER
   $ $ZEPHYR_BASE/../net-tools/tunslip6 -t tapOTHER -T -s /tmp/slip.devOTHER \
        2001:db8::1/64
   $ make run QEMU_INSTANCE=OTHER
