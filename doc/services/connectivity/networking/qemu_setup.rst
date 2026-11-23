.. _networking_with_qemu:

Networking with QEMU
####################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in a QEMU virtual machine (built for Zephyr
targets such as qemu_x86 and qemu_cortex_m3). Some virtual ARM boards (such as
qemu_cortex_a53) only support a single UART, in this case QEMU Ethernet is
preferred, see :ref:`networking_with_eth_qemu` for details.

In this example, the :zephyr:code-sample:`sockets-echo-server` sample application from
the Zephyr source distribution is run in QEMU. The QEMU instance is
connected to a Linux host using a serial port, and SLIP is used to
transfer data between the Zephyr application and Linux (over a chain of
virtual connections).

Prerequisites
*************

On the Linux Host, find the Zephyr `net-tools`_ project, which can either be
found in a Zephyr standard installation under the ``tools/net-tools`` directory
or installed stand alone from its own git repository:

.. code-block:: console

   sudo apt install -y socat libpcap-dev
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
   :zephyr-app: samples/net/sockets/echo_server
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
laptop to normal Wi-Fi use.

To stop the daemons, press Ctrl+C in the corresponding terminal windows
(you need to stop both ``loop-slip-tap.sh`` and ``loop-socat.sh``).

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Accessing the Internet
**********************

To access the internet from a Zephyr application running in QEMU, NAT
(masquerading) has to be set up on the host. See :ref:`networking_internet`
for details.

Network connection between two QEMU VMs
***************************************

Unlike the VM-to-Host setup described above, VM-to-VM setup is
automatic. For sample
applications that support this mode (such as the echo_server and echo_client
samples), you will need two terminal windows, set up for Zephyr development.

Terminal #1:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86
   :goals: build
   :build-args: server
   :compact:

This will start QEMU, waiting for a connection from a client QEMU.

Terminal #2:
============

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
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

   socat PTY,link=/tmp/slip.devMAIN UNIX-LISTEN:/tmp/slip.sockMAIN &
   sudo $ZEPHYR_BASE/../tools/net-tools/tunslip6 -t tapMAIN -T -s /tmp/slip.devMAIN 2001:db8::1/64 &
   # Now run Zephyr
   make -Cbuild run QEMU_INSTANCE=MAIN

Terminal #2:
============

.. code-block:: console

   socat PTY,link=/tmp/slip.devOTHER UNIX-LISTEN:/tmp/slip.sockOTHER &
   sudo $ZEPHYR_BASE/../tools/net-tools/tunslip6 -t tapOTHER -T -s /tmp/slip.devOTHER 2001:db8::1/64 &
   make -Cbuild run QEMU_INSTANCE=OTHER

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
