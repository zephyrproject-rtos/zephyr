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

See the `net-tools`_ project for more details.

This sample can be built and executed on QEMU or native_posix board as
described in :ref:`networking_with_host`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
