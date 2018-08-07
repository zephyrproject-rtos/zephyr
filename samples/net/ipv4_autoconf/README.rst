.. _ipv4-autoconf-sample:

Sample IPv4 autoconf client application
#######################################

Overview
********

This application starts a IPv4 autoconf and self-assigns
a random IPv4 address in the 169.254.0.0/16 range, it defends
the IPv4 address and resolves IPv4 conflicts if multiple
parties try to allocate an identical address

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

These are instructions for how to use this sample application running
on :ref:frdm_k64f board to configure a link local IPv4 address and
connect to a Linux host.

Connect ethernet cable from :ref:Freedom-K64F board <frdm_k64f> to Linux
host machine and check for new interfaces:

Running Avahi client in Linux Host
==================================

Assign a IPv4 link local address to the interface in the Linux system

.. code-block:: console

    $ avahi-autoipd --force-bind -D eth0


FRDM_K64F
=========

Build Zephyr samples/net/ipv4_autoconf application:

.. zephyr-app-commands : :
    :zephyr-app: samples/net/ipv4_autoconf
    :host-os: unix
    :board: frdm_k64f
    :goals: build flash
    :compact:

Once IPv4 LL completed probing and announcement, details are shown like this:

.. code-block:: console

    $ sudo screen /dev/ttyACM0 115200

.. code-block:: console

    [ipv4ll] [INF] main: Run ipv4 autoconf client
    [ipv4ll] [INF] handler: Your address: 169.254.218.128

note that the IP changes at each self assignment

To verify the Zephyr application is running and has configured an ip address
type:

.. code-block:: console

    $ ping -I eth1 169.254.218.128
