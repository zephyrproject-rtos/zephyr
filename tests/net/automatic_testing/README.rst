.. _network-automatic-testing:

Network Automatic Testing
#########################

Overview
********

This test application for Zephyr will setup two virtual LAN networks
and provides echo-server service for normal and encrypted UDP and
TCP connections. The test application also enables net-shell.

The source code for this test application can be found at:
:file:`tests/net/automatic_testing`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

Normally this test application is launched by Zephyr's automatic test system.
It is also possible to run this testing application with QEMU as described in
:ref:`networking_with_qemu`, with native-posix board, or with real hardware.
Note that VLAN is only supported for boards that have an ethernet port.

Follow these steps to build the testing application:

.. zephyr-app-commands::
   :zephyr-app: tests/net/automatic_testing
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

If this application is run in native_posix board, then you will need extra
priviliges to create and configure the TAP device in the host system.
Use the ``sudo`` command to execute the Zephyr process with admin privileges:

.. code-block:: console

    sudo --preserve-env=ZEPHYR_BASE make run

If the sudo command reports an error, then try to execute it like this:

.. code-block:: console

    sudo --preserve-env make run

The default configuration file prj.conf creates two virtual LAN networks
with these settings and one normal ethernet interface:

- VLAN tag 100: IPv4 198.51.100.1 and IPv6 2001:db8:100::1
- VLAN tag 200: IPv4 203.0.113.1 and IPv6 2001:db8:200::1
- IPv4 192.0.2.1 and IPv6 2001:db8::1

Setting up Linux Host
=====================

The :file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that can be
executed on the Linux host. It creates two VLANs on the Linux host and
suitable IP routes to Zephyr. This script is not needed for a native-posix board
as the host IP address setup is done automatically for it.

If everything is configured correctly, you will be able to successfully execute
the following commands on the Linux host.

.. code-block:: console

    ping -c 1 2001:db8:100::1
    ping -c 1 198.51.100.1
    ping -c 1 2001:db8:200::1
    ping -c 1 203.0.113.1
    ping -c 1 192.0.2.1
    ping -c 1 2001:db8::1

You can also execute the ``echo-client`` program found in the
`net-tools`_ project.

.. code-block:: console

    ./echo-client 2001:db8::1
    ./echo-client 2001:db8:100::1
    ./echo-client 2001:db8:200::1

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
