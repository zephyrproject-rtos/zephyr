.. _tc-sample:

Traffic Class Sample Application
################################

Overview
********

The TC sample application for Zephyr sets up two virtual LAN networks and
starts to send prioritized UDP packets from one VLAN network to the other.
The use of VLAN network is optional but as VLAN defines priorities to network
traffic, it is convenient to use that concept here. The application also
enables net-shell and allows user to view VLAN settings.

The source code for this sample application can be found at:
:file:`samples/net/traffic_class`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

A good way to run this TC application is with QEMU as described in
:ref:`networking_with_qemu`.
Currently only one VLAN network (tag) is supported when Zephyr is run inside
QEMU. If you're using a FRDM-K64F board, then multiple VLAN networks can be
configured. Note that VLAN is only supported for boards that have ethernet
port.

Follow these steps to build the TC sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/traffic_class
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

The default configuration file prj.conf creates two virtual LAN networks
with these settings:

- VLAN tag 100: IPv4 198.51.100.1 and IPv6 2001:db8:100::1
- VLAN tag 200: IPv4 203.0.113.1 and IPv6 2001:db8:200::1

Setting up Linux Host
=====================

The :file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that can be
executed on the Linux host. It creates two VLANs on the Linux host and creates
routes to Zephyr.

If everything is configured correctly, you will be able to successfully execute
the following commands on the Linux host.

.. code-block:: console

    ping -c 1 2001:db8:100::1
    ping -c 1 198.51.100.1
    ping -c 1 2001:db8:200::1
    ping -c 1 203.0.113.1

Running echo-server in Linux Host
=================================

There is one useful testing scenario that can be used with Linux host.
Here traffic-class application is run in QEMU and echo-server is run in
Linux host.

In a terminal window:

.. code-block:: console

    $ sudo ./echo-server -i tap0

Then you can run traffic-class application in QEMU as described above.
