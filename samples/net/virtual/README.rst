.. zephyr:code-sample:: virtual-network-interface
   :name: Virtual network interface
   :relevant-api: virtual virtual_mgmt

   Create a sample virtual network interface.

Overview
********

This sample application creates a sample virtual network interface for
demonstrative purposes, it does not do anything useful here.
There are total 4 network interfaces.

Ethernet network interface is providing the real network interface and
all the virtual interfaces are running on top of it.

On top of Ethernet interface there are two virtual network interfaces,
one provides only IPv6 tunnel, and the other only IPv4. These two tunnels
are provided by IPIP tunnel.

The sample provides tunnel interface which runs on top of the IPv6 tunnel.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/virtual`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

The `net-tools`_ project provides a configuration that can be used
to create a sample tunnels in host side.

.. code-block:: console

	net-setup.sh -c zeth-tunnel.conf

Note that the sample application expects that the board provides
an Ethernet network interface. Build the sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/virtual
   :board: <board to use>
   :goals: build
   :compact:

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
