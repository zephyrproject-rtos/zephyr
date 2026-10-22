.. zephyr:code-sample:: packet-socket
   :name: Packet socket
   :relevant-api: bsd_sockets ethernet

   Use raw packet sockets over Ethernet.

Overview
********

This sample is a simple packet socket application showing usage
of packet sockets over Ethernet. The sample prints every packet
received, and sends a dummy packet every 5 seconds.
The Zephyr network subsystem does not touch any of the headers
(L2, L3, etc.).

Building and Running
********************

When the application is run, it opens a packet socket and prints
the length of the packet it receives. After that it sends a dummy
packet every 5 seconds. You can use Wireshark to observe these
sent and received packets.

The receiving socket also joins an Ethernet multicast group so that the
network interface starts to listen to one extra multicast MAC address. This
is done with the ``PACKET_ADD_MEMBERSHIP`` socket option, and the group is
left again with ``PACKET_DROP_MEMBERSHIP`` when the sample stops. Set
:kconfig:option:`CONFIG_NET_SAMPLE_MCAST_ADDR` to pick the address, or
disable :kconfig:option:`CONFIG_NET_SAMPLE_MCAST_MEMBERSHIP` to skip
joining altogether.

The join and the leave are also reported as network management events, which
can be seen from the shell with:

.. code-block:: console

   uart:~$ net events on

See the `net-tools`_ project for more details.

This sample can be built and executed on QEMU or native_sim board as
described in :ref:`networking_with_host`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
