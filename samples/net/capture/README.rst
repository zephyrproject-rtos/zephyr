.. _net-capture-sample:

Network Packet Capture
######################

Overview
********

This application will setup the device so that net-shell can be used
to enable network packet capture. The captured packets are sent to
remote host via IPIP tunnel. The tunnel can be configured to be in the
same connection as what we are capturing packets or it can be a separate
bearer. For example if you are capturing network traffic for interface 1,
then the remote host where the captured packets are sent can also be reached
via interface 1 or via some other network interface if the device has
multiple network interfaces connected.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

Build the sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/capture
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:


Network Configuration
*********************

The ``net-tools`` project contains ``net-setup.sh`` script that can be used to setup
the tunneling.

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh -c zeth-tunnel.conf

The script will create following network interfaces:

.. code-block:: console

   zeth: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        inet 192.0.2.2  netmask 255.255.255.255  broadcast 0.0.0.0
        inet6 2001:db8::2  prefixlen 128  scopeid 0x0<global>
        ether 00:00:5e:00:53:ff  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   zeth-ip6ip: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1480
        inet6 2001:db8:200::2  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::c000:202  prefixlen 64  scopeid 0x20<link>
        sit  txqueuelen 1000  (IPv6-in-IPv4)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

   zeth-ip6ip6: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1452
        inet6 fe80::486c:eeff:fead:5d11  prefixlen 64  scopeid 0x20<link>
        inet6 2001:db8:100::2  prefixlen 64  scopeid 0x0<global>
        unspec 20-01-0D-B8-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 8  dropped 8 overruns 0  carrier 8  collisions 0

   zeth-ipip: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1480
        inet 198.51.100.2  netmask 255.255.255.0  destination 198.51.100.2
        inet6 fe80::5efe:c000:202  prefixlen 64  scopeid 0x20<link>
        tunnel   txqueuelen 1000  (IPIP Tunnel)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 7  dropped 0 overruns 0  carrier 0  collisions 0

   zeth-ipip6: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1452
        inet 203.0.113.2  netmask 255.255.255.0  destination 203.0.113.2
        inet6 fe80::387b:a6ff:fe56:6cac  prefixlen 64  scopeid 0x20<link>
        unspec 20-01-0D-B8-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 7  dropped 7 overruns 0  carrier 0  collisions 0

The ``zeth`` is the outer tunnel interface, all the packets go via it.
The other interfaces receive packets depending on the configuration you have
in the Zephyr side.

Network Capture Configuration
=============================

In Zephyr console, type:

.. code-block:: console

   uart:~$ net iface

   Interface 0x807df74 (Virtual) [1]
   =================================
   Interface is down.

   Interface 0x807e040 (Ethernet) [2]
   ==================================
   Link addr : 02:00:5E:00:53:3B
   MTU       : 1452
   Flags     : AUTO_START,IPv4,IPv6
   Ethernet capabilities supported:
   IPv6 unicast addresses (max 4):
        fe80::5eff:fe00:533b autoconf preferred infinite
        2001:db8::1 manual preferred infinite
   IPv6 multicast addresses (max 4):
        ff02::1
        ff02::1:ff00:533b
        ff02::1:ff00:1
   IPv6 prefixes (max 2):
        <none>
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 43300
   IPv6 retransmit timer    : 0
   IPv4 unicast addresses (max 2):
        192.0.2.1 manual preferred infinite
   IPv4 multicast addresses (max 1):
        <none>
   IPv4 gateway : 0.0.0.0
   IPv4 netmask : 255.255.255.0

Next the monitoring is setup so that captured packets are sent as a payload
in IPv6/UDP packets.

.. code-block:: console

   uart:~$ net capture setup 192.0.2.2 2001:db8:200::1 2001:db8:200::2
   Capture setup done, next enable it by "net capture enable <idx>"

The ``net capture`` command will show current configuration. As we have not
yet enabled capturing, the interface is not yet set.

.. code-block:: console

   uart:~$ net capture
   Network packet capture disabled
                   Capture  Tunnel
   Device          iface    iface   Local                  Peer
   NET_CAPTURE0    -        1      [2001:db8:200::1]:4242  [2001:db8:200::2]:4242

Next enable network packet capturing for interface 2.

.. code-block:: console

   uart:~$ net capture enable 2

The tunneling interface will be UP and the captured packets will be sent to
peer host.

.. code-block:: console

   uart:~$ net iface 1

   Interface 0x807df74 (Virtual) [1]
   =================================
   Name      : IPv4 tunnel
   Attached  : 2 (Ethernet / 0x807e040)
   Link addr : 8E:F9:94:6D:B9:E6
   MTU       : 1452
   Flags     : POINTOPOINT,NO_AUTO_START,IPv6
   IPv6 unicast addresses (max 4):
        fe80::aee6:fbff:fe50:28c0 autoconf preferred infinite
        2001:db8:200::1 manual preferred infinite
   IPv6 multicast addresses (max 4):
        <none>
   IPv6 prefixes (max 2):
        <none>
   IPv6 hop limit           : 64
   IPv6 base reachable time : 30000
   IPv6 reachable time      : 22624
   IPv6 retransmit timer    : 0
   IPv4 not enabled for this interface.

If you now do this:

.. code-block:: console

   uart:~$ net ping -c 1 192.0.2.2

You should see a ICMPv4 message sent to ``192.0.2.2`` and also the captured
packet will be sent to ``192.0.2.2`` in tunnel to ``2001:db8:200::2``
address. The UDP port is by default ``4242`` but that can be changed when
setting the tunnel endpoint address.

The actual captured network packets received at the end of the tunnel will look
like this:

.. code-block:: console

   No.     Time           Source                Destination           Protocol Length Info
        34 106.078538049  192.0.2.1             192.0.2.2             ICMP     94     Echo (ping) request  id=0xdc36, seq=0/0, ttl=64 (reply in 35)

   Frame 34: 94 bytes on wire (752 bits), 94 bytes captured (752 bits) on interface zeth-ip6ip, id 0
   Raw packet data
   Internet Protocol Version 6, Src: 2001:db8:200::1, Dst: 2001:db8:200::2
   User Datagram Protocol, Src Port: 4242, Dst Port: 4242
   Ethernet II, Src: 02:00:5e:00:53:3b (02:00:5e:00:53:3b), Dst: ICANNIAN_00:53:ff (00:00:5e:00:53:ff)
   Internet Protocol Version 4, Src: 192.0.2.1, Dst: 192.0.2.2
   Internet Control Message Protocol

   No.     Time           Source                Destination           Protocol Length Info
        35 106.098850599  192.0.2.2             192.0.2.1             ICMP     94     Echo (ping) reply    id=0xdc36, seq=0/0, ttl=64 (request in 34)

   Frame 35: 94 bytes on wire (752 bits), 94 bytes captured (752 bits) on interface zeth-ip6ip, id 0
   Raw packet data
   Internet Protocol Version 6, Src: 2001:db8:200::1, Dst: 2001:db8:200::2
   User Datagram Protocol, Src Port: 4242, Dst Port: 4242
   Ethernet II, Src: ICANNIAN_00:53:ff (00:00:5e:00:53:ff), Dst: 02:00:5e:00:53:3b (02:00:5e:00:53:3b)
   Internet Protocol Version 4, Src: 192.0.2.2, Dst: 192.0.2.1
   Internet Control Message Protocol
