.. zephyr:code-sample:: net-pkt-filter
   :name: Network packet filter
   :relevant-api: net_pkt_filter

   Install network packet filter hooks.

Overview
********

This sample shows how to set network packet filters from a user application.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/pkt_filter`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

A good way to run this sample application is with QEMU or native_sim board
as described in :ref:`networking_with_host`.

For demo purposes, the VLAN support needs to be enabled in host side like this.
Execute these commands in a terminal window:

.. code-block:: console

   $ cd tools/net-tools
   $ ./net-setup.sh  -c zeth-vlan.conf

Then follow these steps to build the network packet filter sample application for
either ``qemu_x86`` or ``native_sim`` boards:

.. zephyr-app-commands::
   :zephyr-app: samples/net/pkt_filter
   :board: <board to use>
   :conf: "prj.conf overlay-vlan.conf"
   :goals: build
   :compact:

In this example, we enable VLAN support with these settings:

The VLAN overlay configuration file :zephyr_file:`samples/net/pkt_filter/overlay-vlan.conf`
creates two virtual LAN networks with these settings:

- VLAN tag 100: IPv4 198.51.100.1 and IPv6 2001:db8:100::1
- VLAN tag 200: IPv4 203.0.113.1 and IPv6 2001:db8:200::1

In network shell, you can monitor the network packet filters:

.. code-block:: console

   uart:~$ net filter
   Rule  Type        Verdict  Tests
   [ 1]  recv        OK       3    eth vlan type[0x0800],size max[200],iface[2]
   [ 2]  recv        OK       3    eth vlan type[0x0800],size min[100],iface[3]
   [ 3]  recv        OK       1    iface[1]
   [ 4]  recv        OK       2    eth vlan type[0x0806],iface[2]
   [ 5]  recv        OK       2    eth vlan type[0x0806],iface[3]
   [ 6]  recv        DROP     0

The above sample application network packet filter rules can be interpreted
like this:

* Rule 1: Allow IPv4 (Ethernet type 0x0800) packets with max size 200 bytes
  to network interface 2 which is the first VLAN interface.

* Rule 2: Allow IPv4 packets with min size 100 bytes to network interface 3
  which is the second VLAN interface.

* Rule 3: Allow all incoming traffic to Ethernet interface 1

* Rule 4: Allow ARP packets (Ethernet type 0x0806) to VLAN interface 2

* Rule 5: Allow ARP packets (Ethernet type 0x0806) to VLAN interface 3

* Rule 6: Drop all other packets. This also means that IPv6 packets are
  dropped.

The network statistics can be used to see that the packets are dropped.
Use ``net stats`` command to monitor statistics.

You can verify the rules from network shell:

.. code-block:: console

   uart:~$ net ping 2001:db8:100::2 -c 2
   PING 2001:db8:100::2
   Ping timeout
   uart:~$ net stats 2
   Interface 0x8089c6c (Virtual) [2]
   ==================================
   IPv6 recv      0        sent    3       drop    0       forwarded       0
   IPv6 ND recv   0        sent    7       drop    1
   IPv6 MLD recv  0        sent    0       drop    0
   ICMP recv      0        sent    3       drop    0
   ...
   Filter drop rx 10       tx      0
   Bytes received 320
   Bytes sent     660
   Processing err 10

   uart:~$ net ping 198.51.100.2 -c 1
   PING 198.51.100.2
   28 bytes from 198.51.100.2 to 198.51.100.1: icmp_seq=1 ttl=64 time=100 ms

   uart:~$ net ping 198.51.100.2 -c 1 -s 201
   PING 198.51.100.2
   Ping timeout

   uart:~$ net ping 203.0.113.2 -c 1
   PING 203.0.113.2
   Ping timeout

   uart:~$ net ping 203.0.113.2 -c 1 -s 101
   PING 203.0.113.2
   125 bytes from 203.0.113.2 to 203.0.113.1: icmp_seq=1 ttl=64 time=20 ms
