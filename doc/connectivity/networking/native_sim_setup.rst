.. _networking_with_native_sim:

Networking with native_sim board
################################

.. contents::
    :local:
    :depth: 2

Using virtual/TAP Ethernet driver
*********************************

This paragraph describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in a :ref:`native_sim <native_sim>` board.

In this example, the :zephyr:code-sample:`sockets-echo-server` sample application from
the Zephyr source distribution is run in native_sim board. The Zephyr
native_sim board instance is connected to a Linux host using a tuntap device
which is modeled in Linux as an Ethernet network interface.

Prerequisites
=============

On the Linux Host, find the Zephyr `net-tools`_ project, which can either be
found in a Zephyr standard installation under the ``tools/net-tools`` directory
or installed stand alone from its own git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools


Basic Setup
===========

For the steps below, you will need three terminal windows:

* Terminal #1 is terminal window with net-tools being the current
  directory (``cd net-tools``)
* Terminal #2 is your usual Zephyr development terminal,
  with the Zephyr environment initialized.
* Terminal #3 is the console to the running Zephyr native_sim
  instance (optional).

Step 1 - Create Ethernet interface
----------------------------------

Before starting native_sim with network emulation, a network interface
should be created.

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh

You can tweak the behavior of the net-setup.sh script. See various options
by running ``net-setup.sh`` like this:

.. code-block:: console

   ./net-setup.sh --help


Step 2 - Start app in native_sim board
--------------------------------------

Build and start the ``echo_server`` sample application.

In terminal #2, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:


Step 3 - Connect to console (optional)
--------------------------------------

The console window should be launched automatically when the Zephyr instance is
started but if it does not show up, you can manually connect to the console.
The native_sim board will print a string like this when it starts:

.. code-block:: console

   UART connected to pseudotty: /dev/pts/5

You can manually connect to it like this:

.. code-block:: console

   screen /dev/pts/5

Using offloaded sockets
***********************

The main advantage over `Using virtual/TAP Ethernet driver`_ is not needing to
setup a virtual network interface on the host machine. This means that no
leveraged (root) privileges are needed.

Step 1 - Start app in native_sim board
======================================

Build and start the ``echo_server`` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=overlay-nsos.conf
   :goals: run
   :compact:

Step 2 - run echo-client from net-tools
=======================================

On the Linux Host, find the Zephyr `net-tools`_ project, which can either be
found in a Zephyr standard installation under the ``tools/net-tools`` directory
or installed stand alone from its own git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools

.. note::

   Native Simulator with the offloaded sockets network driver is using the same
   network interface/namespace as any other (Linux) application that uses BSD
   sockets API. This means that :zephyr:code-sample:`sockets-echo-server` and
   ``echo-client`` applications will communicate over localhost/loopback
   interface (address ``127.0.0.1``).

To run UDP test, type:

.. code-block:: console

   ./echo-client 127.0.0.1

For TCP test, type:

.. code-block:: console

   ./echo-client -t 127.0.0.1

Setting interface name and IPv4 parameters from command line
************************************************************

By default the Ethernet interface name used by native_sim is determined by
:kconfig:option:`CONFIG_ETH_NATIVE_TAP_DRV_NAME`, but is also possible
to set it from the command line using ``--eth-if=<interface_name>``.

The same applies to the IPv4 address, gateway and netmask. They can be set
from the command line using ``--ipv4-addr=<ip_address>``,
``--ipv4-gw=<gateway>`` and ``--ipv4-nm=<netmask>``.

Note that the configuration :kconfig:option:`CONFIG_NET_CONFIG_MY_IPV4_ADDR`
and the command line arguments work in parallel. This means that if both are
set, the interface might end up with two IP addresses. In most cases, it does
make sense to only use one of both at the same time.

This can be useful if the application has to be
run in multiple instances and recompiling it for each instance would be
troublesome.

.. code-block:: console

   ./zephyr.exe --eth-if=zeth2 --ipv4-addr=192.0.2.2 --ipv4-gw=192.0.0.1
   --ipv4-nm=255.255.0.0

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
