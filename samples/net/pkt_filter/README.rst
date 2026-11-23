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
   :conf: "prj.conf vlan.conf"
   :goals: build
   :compact:

In this example, we enable VLAN support with these settings:

The VLAN extra config file :zephyr_file:`samples/net/pkt_filter/vlan.conf`
creates two virtual LAN networks with these settings:

- VLAN tag 100: IPv4 198.51.100.1 and IPv6 2001:db8:100::1
- VLAN tag 200: IPv4 203.0.113.1 and IPv6 2001:db8:200::1

In network shell, you can monitor the network packet filters:

.. code-block:: console

   uart:~$ net filter
   Rule  Type        Verdict   Pkt-Prio  Queue  Thread-Prio  Tests
   [ 1]  recv        OK             N/A    N/A          N/A  3    iface[2],eth vlan type[0x0800],size max[200]
   [ 2]  recv        OK             N/A    N/A          N/A  3    iface[3],eth vlan type[0x0800],size min[100]
   [ 3]  recv        OK             N/A    N/A          N/A  1    iface[1]
   [ 4]  recv        OK             N/A    N/A          N/A  2    iface[2],eth vlan type[0x0806]
   [ 5]  recv        OK             N/A    N/A          N/A  2    iface[3],eth vlan type[0x0806]
   [ 6]  recv        DROP           N/A    N/A          N/A  0
   [ 7]  local recv  OK             N/A    N/A          N/A  1    local_in[fn=0x804a755, data=0x809204c]
   [ 8]  local recv  DROP           N/A    N/A          N/A  2    iface[3],udp dst port[4242]
   [ 9]  local recv  DROP           N/A    N/A          N/A  2    tcp flags[0x02/0x12],tcp dst port[8080]
   [10]  local recv  DROP           N/A    N/A          N/A  3    udp dst port[5000],ip src block[203.0.113.2],ip src block[2001:db8:200::2]
   [11]  local recv  OK             N/A    N/A          N/A  0
   [12]  IPv4 recv   OK             N/A    N/A          N/A  1    ip src block[192.0.2.2,198.51.100.3]
   [13]  IPv6 recv   OK             N/A    N/A          N/A  1    ip src block[2001:db8::2,2001:db8::100:2]

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

* Rule 7: Call the ``handler_local_in()`` function of the sample for each received UDP or TCP
  packet. The function counts the packets and returns false, so this rule does not match any
  packet and the rules that follow decide what happens to it.

* Rule 8: Drop UDP datagrams to port 4242 received from network interface 3, which is the
  second VLAN interface.

* Rule 9: Drop TCP connection requests to port 8080. A connection request is a TCP segment
  with the SYN flag (0x02) set and the ACK flag (0x10) clear.

* Rule 10: Accept UDP datagrams to port 5000 from one host only, the peer on the second VLAN,
  by dropping the ones that are not from ``203.0.113.2`` or ``2001:db8:200::2``. Each address
  condition only restricts the packets of its address family and is true for the packets of
  the other one, so one rule can have a condition for the IPv4 and one for the IPv6 address.

* Rule 11: Accept all other packets.

* Rule 12: Drop IPv4 packets where the source address is either ``192.0.2.2`` or
  ``198.51.100.3``.

* Rule 13: Drop IPv6 packets where the source address is either ``2001:db8::2`` or
  ``2001:db8::100:2``.

If you enable network packet priority option :kconfig:option:`CONFIG_NET_SAMPLE_USE_PACKET_PRIORITIES`
then the sample will install extra rules for setting up the priorities.

   uart:~$ net filter
   Rule  Type        Verdict   Pkt-Prio  Queue  Thread-Prio  Tests
   [ 1]  recv        CONTINUE         1      0            1  1    iface[1]
   [ 2]  recv        CONTINUE         7      2         SKIP  2    iface[1],eth type[0x88f7]
   [ 3]  recv        CONTINUE         2      0            1  2    iface[1],eth type[0x8100]
   [ 4]  recv        CONTINUE         1      0            1  2    iface[2],eth vlan type[0x0806]
   [ 5]  recv        CONTINUE         1      0            1  2    iface[3],eth vlan type[0x0806]
   [ 6]  recv        OK             N/A    N/A          N/A  3    iface[2],eth vlan type[0x0800],size max[200]
   [ 7]  recv        OK             N/A    N/A          N/A  3    iface[3],eth vlan type[0x0800],size min[100]
   [ 8]  recv        OK             N/A    N/A          N/A  1    iface[1]
   [ 9]  recv        OK             N/A    N/A          N/A  2    iface[2],eth vlan type[0x0806]
   [10]  recv        OK             N/A    N/A          N/A  2    iface[3],eth vlan type[0x0806]
   [11]  recv        DROP           N/A    N/A          N/A  0
   [12]  local recv  OK             N/A    N/A          N/A  1    local_in[fn=0x804a755, data=0x809324c]
   [13]  local recv  DROP           N/A    N/A          N/A  2    iface[3],udp dst port[4242]
   [14]  local recv  DROP           N/A    N/A          N/A  2    tcp flags[0x02/0x12],tcp dst port[8080]
   [15]  local recv  DROP           N/A    N/A          N/A  3    udp dst port[5000],ip src block[203.0.113.2],ip src block[2001:db8:200::2]
   [16]  local recv  OK             N/A    N/A          N/A  0
   [17]  IPv4 recv   OK             N/A    N/A          N/A  1    ip src block[192.0.2.2,198.51.100.3]
   [18]  IPv6 recv   OK             N/A    N/A          N/A  1    ip src block[2001:db8::2,2001:db8::100:2]

The above sample application network packet filter rules can be interpreted
like this:

* Rules 1 - 5: Add rules to set network packet priority to certain type packets.

* Rule 6 - 18: These are the same as in previous rule list.

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

The UDP and TCP rules can be verified from the host, for example with ``nc``. A UDP datagram
that the host sends to port 4242 of ``203.0.113.1`` is dropped without an answer, while one to
another port is answered with an ICMP port unreachable message, as no application listens to
it. A tool like ``tcpdump`` on the ``vlan.200`` interface of the host shows the answer. The
datagram needs at least 54 bytes of data to get past rule 2:

.. code-block:: console

   $ head -c 64 /dev/zero | nc -u -w1 203.0.113.1 4242
   $ head -c 64 /dev/zero | nc -u -w1 203.0.113.1 4243

A datagram to port 5000 of that address is answered as well, because the host sends it from
``203.0.113.2``, but one to port 5000 of ``198.51.100.1`` is not, because the host sends it
from ``198.51.100.2``:

.. code-block:: console

   $ head -c 64 /dev/zero | nc -u -w1 203.0.113.1 5000
   $ head -c 64 /dev/zero | nc -u -w1 198.51.100.1 5000

In the same way, a TCP connection request to port 8080 of ``198.51.100.1`` is not answered, so
the first of these commands reports a timeout, while one to another port is answered with a
reset, so the second one reports that the connection is refused:

.. code-block:: console

   $ nc -zv -w3 198.51.100.1 8080
   $ nc -zv -w3 198.51.100.1 8081

The dropped packets are counted in the local filter statistics of the interface they are
received from:

.. code-block:: console

   uart:~$ net stats 3
   Interface 0x8091ec8 (Virtual) [3]
   ==================================
   ...
   Filter drop rx 0        IPv4    0       IPv6    0       local   1       tx      0
