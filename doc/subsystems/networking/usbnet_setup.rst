.. _usb_device_networking_setup:

USB Device Networking
#####################

This page describes how to set up networking between a Linux host
and a Zephyr application running on USB supported devices.

The board is connected to Linux host using USB cable
and provides an Ethernet interface to the host.
The :ref:`echo-server-sample` application from the Zephyr source
distribution is run on supported board.  The board is connected to a
Linux host using a USB cable providing an Ethernet interface to the host.

Basic Setup
***********

To communicate with the Zephyr application over a newly created Ethernet
interface, we need to assign IP addresses and set up a routing table for
the Linux host.
After plugging a USB cable from the board to the Linux host, the
``cdc_ether`` driver registers a new Ethernet device with a provided MAC
address.

You can check that network device is created and MAC address assigned by
running dmesg from the Linux host.

.. code-block:: console

   cdc_ether 1-2.7:1.0 eth0: register 'cdc_ether' at usb-0000:00:01.2-2.7, CDC Ethernet Device, 00:00:5e:00:53:01

We need to set it up and assign IP addresses as explained in the following
section.

Choosing IP addresses
=====================

To establish network connection to the board we need to choose IP address
for the interface on the Linux host.

It make sense to choose addresses in the same subnet we have in Zephyr
application. IP addresses usually set in the project configuration files
and may be checked also from the shell with following commands. Connect
a serial console program (such as puTTY) to the board, and enter this
command to the Zephyr shell:

.. code-block:: console

   shell> net iface

   Interface 0xa800e580 (Ethernet)
   ===============================
   Link addr : 00:00:5E:00:53:00
   MTU       : 1500
   IPv6 unicast addresses (max 2):
           fe80::200:5eff:fe00:5300 autoconf preferred infinite
           2001:db8::1 manual preferred infinite
   ...
   IPv4 unicast addresses (max 1):
           192.0.2.1 manual preferred infinite

This command shows that one IPv4 address and two IPv6 addresses have
been assigned to the board. We can use either IPv4 or IPv6 for network
connection depending on the board network configuration.

Next step is to assign IP addresses to the new Linux host interface, in
the following steps ``enx00005e005301`` is the name of the interface on my
Linux system.

Setting IPv4 address and routing
================================

.. code-block:: console

   # ip address add dev enx00005e005301 192.0.2.2
   # ip link set enx00005e005301 up
   # ip route add 192.0.2.0/24 dev enx00005e005301

Setting IPv6 address and routing
================================

.. code-block:: console

   # ip address add dev enx00005e005301 2001:db8::2
   # ip link set enx00005e005301 up
   # ip -6 route add 2001:db8::/64 dev enx00005e005301

Testing connection
******************

From the host we can test the connection by pinging Zephyr IP address of
the board with:

.. code-block:: console

   $ ping 192.0.2.1
   PING 192.0.2.1 (192.0.2.1) 56(84) bytes of data.
   64 bytes from 192.0.2.1: icmp_seq=1 ttl=64 time=2.30 ms
   64 bytes from 192.0.2.1: icmp_seq=2 ttl=64 time=1.43 ms
   64 bytes from 192.0.2.1: icmp_seq=3 ttl=64 time=2.45 ms
   ...
