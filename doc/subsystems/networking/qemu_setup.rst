.. _networking_with_qemu:

Networking with Qemu
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

   $ git clone https://gerrit.zephyrproject.org/r/net-tools
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

Before starting QEMU with network emulation, a unix socket for the emulation
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

.. code-block:: console

   $ cd samples/net/echo_server
   $ make pristine && make qemu

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

Setting up NAT/masquerading to access Internet
**********************************************

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

.. code-block:: console

   $ cd samples/net/echo_server
   $ make server

This will start QEMU, waiting for connection from a client QEMU.

Terminal #2:
============

.. code-block:: console

   $ cd samples/net/echo_client
   $ make client

This will start 2nd QEMU instance, and you should see logging of data sent and
received in both.
