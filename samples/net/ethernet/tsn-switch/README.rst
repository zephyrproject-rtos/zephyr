.. zephyr:code-sample:: tsn-switch
   :name: TSN Switch
   :relevant-api: eth_bridge ptp_clock_interface ethernet_mgmt

   Test and debug TSN switch

Overview
********

Example on testing/debugging TSN switch.

In Zephyr, switch device is supported with DSA (Distributed Switch Architecture) driver.
And Ethernet bridge over DSA user ports will be used as a way for switch configuration,
like FDB, VLAN ... (Unfortunately these haven't been supported.)

For TSN protocols, gPTP stack now can be enabled on DSA user ports to make the switch
as time-aware relay. Qbv can be managed by management APIs. More TSN protocols will be
supported and can be enabled in this sample.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ethernet/tsn-switch`.

Requirements
************

To verify TSN switch, at least two Linux hosts supporting TSN (at least gPTP) are needed
to connect to the switch.

Building and Running
********************

1. Build and run the sample.

.. zephyr-app-commands::
   :zephyr-app: samples/net/ethernet/tsn-switch
   :board: <board to use>
   :goals: build
   :compact:

2. Connect Linux hosts to TSN switch. Take NXP i.MX943 EVK as example.

.. code-block:: none

                         i.MX943 EVK
                     +-----------------+
                     |     bridge0     | gPTP bridge as Master
                     |  IP: 192.0.2.1  |    priority1 246
                     |                 |
                     | swp0(Qbv)  swp1 |
                     +--+----------+---+
                        |          |
                +-------+          +-------+
                |                          |
   +------------+-----------+  +-----------+------------+
   |          eth0          |  |          eth0          |
   | IP:  192.0.2.2         |  | IP:  192.0.2.3         |
   | MAC: fa:fa:62:fe:a2:11 |  | MAC: a6:10:52:1d:cb:6d |
   +------------------------+  +------------------------+
         Linux Host A                Linux Host B
    gPTP Slave priority1 248    gPTP slave priority1 248

The Qbv is configured for demo on first DSA user port swp0.

.. code-block:: none

   time cycle 10ms   gate7 6 5 4 3 2 1 0
   -----------------------------------------
   slot 1 - 2ms:         1 1 1 1 0 1 0 1
   slot 2 - 8ms:         1 1 1 1 1 0 0 1

With IP addresses configured on Linux hosts. The basic communication among three devices can be
verified with ping.

3. Run gPTP (ptp4l with gPTP.cfg) on Linux hosts to synchronize time base to switch.

.. code-block:: console

   # ptp4l -i eth0 -m -f gPTP-slave.cfg > /tmp/ptp.log &
   # tail -f /tmp/ptp.log
   ptp4l[186.753]: rms   25 max   51 freq  -8579 +/-  32 delay  1311 +/-   0
   ptp4l[187.752]: rms   19 max   39 freq  -8575 +/-  26 delay  1311 +/-   0
   ptp4l[188.752]: rms   30 max   59 freq  -8592 +/-  41 delay  1311 +/-   0
   ptp4l[189.752]: rms   17 max   28 freq  -8570 +/-  22 delay  1311 +/-   0
   ptp4l[190.752]: rms   23 max   37 freq  -8600 +/-  25 delay  1314 +/-   0
   ptp4l[191.752]: rms   24 max   42 freq  -8600 +/-  32 delay  1310 +/-   0
   ptp4l[192.752]: rms   30 max   54 freq  -8574 +/-  38 delay  1310 +/-   0
   ptp4l[193.753]: rms   25 max   35 freq  -8587 +/-  35 delay  1314 +/-   0
   ptp4l[194.752]: rms   27 max   48 freq  -8598 +/-  35 delay  1314 +/-   0
   ptp4l[195.752]: rms   26 max   42 freq  -8602 +/-  35 delay  1314 +/-   0
   ptp4l[196.752]: rms   25 max   42 freq  -8588 +/-  34 delay  1318 +/-   0

4. Send packets from Linux host B to A with pktgen to verify Qbv function.

On Linux host B, send VLAN 100 PCP 0 packets every second.

.. code-block:: console

   # modprobe pktgen
   # cd /proc/net/pktgen
   # echo "add_device eth0"           > kpktgend_0
   # echo "count 0"                   > eth0
   # echo "clone_skb 0"               > eth0
   # echo "pkt_size 64"               > eth0
   # echo "dst_mac fa:fa:62:fe:a2:11" > eth0
   # echo "vlan_id 100"               > eth0
   # echo "vlan_p 0"                  > eth0
   # echo "delay 1000000000"          > eth0
   # echo "start" > pgctrl


On Linux host A, capture packets. Because gate 0 always on, packets can be received.

.. code-block:: console

   # tcpdump -i eth0 -e vlan
   tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
   listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
   13:56:12.259668 a6:10:52:1d:cb:6d (oui Unknown) > fa:fa:62:fe:a2:11 (oui Unknown), ethertype 802.1Q (0x8100), length 64: vlan 100, p 0, ethertype IPv4, 169.254.153.102.9 > 0.0.0.0.9: UDP, length 18
   13:56:13.259696 a6:10:52:1d:cb:6d (oui Unknown) > fa:fa:62:fe:a2:11 (oui Unknown), ethertype 802.1Q (0x8100), length 64: vlan 100, p 0, ethertype IPv4, 169.254.153.102.9 > 0.0.0.0.9: UDP, length 18
   13:56:14.259714 a6:10:52:1d:cb:6d (oui Unknown) > fa:fa:62:fe:a2:11 (oui Unknown), ethertype 802.1Q (0x8100), length 64: vlan 100, p 0, ethertype IPv4, 169.254.153.102.9 > 0.0.0.0.9: UDP, length 18

On Linux host B, change to send VLAN 100 PCP 1 packets every second.

.. code-block:: console

   # echo "vlan_p 1" > eth0
   # echo "start" > pgctrl


On Linux host A, capture packets. Because gate 1 always off, no packets can be received.

.. code-block:: console

   # tcpdump -i eth0 -e vlan
   tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
   listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes

5. Send packets from Linux host B to A with pktgen to verify Qbv performance.

On Linux host B, send VLAN 100 PCP 2 packets every 100us.

.. code-block:: console

   # modprobe pktgen
   # cd /proc/net/pktgen
   # echo "add_device eth0"           > kpktgend_0
   # echo "count 0"                   > eth0
   # echo "clone_skb 0"               > eth0
   # echo "pkt_size 64"               > eth0
   # echo "dst_mac fa:fa:62:fe:a2:11" > eth0
   # echo "vlan_id 100"               > eth0
   # echo "vlan_p 2"                  > eth0
   # echo "delay 100000"              > eth0
   # echo "start" > pgctrl


On Linux host A, capture 100000 packets and check if they fell in 2ms window.
Because gate 2 has 2ms slot, packets can be received only in 2ms window.
The results showed 99.98% were in 2ms window considering some critical condition.

.. code-block:: none

   # tcpdump -i eth0 -j adapter_unsynced --time-stamp-precision nano -tt -e vlan -c 100000 -w pcp2.pcap
   tcpdump: listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
   100000 packets captured
   101793 packets received by filter
   0 packets dropped by kernel
   #
   #
   # tcpdump -r pcp2.pcap --time-stamp-precision nano -tt -n 2>/dev/null | \
   awk '
   BEGIN{
     period=10000000; win=2000000; off=0;
     win_in=0; win_out=0; total=0;
   }
   $1 ~ /^[0-9]+\.[0-9]+$/ {
     split($1,a,"."); t=a[1]*1000000000 + (a[2]+0);
     phase = ((t - off) % period + period) % period;
     if (phase < win) win_in++; else win_out++;
     total++;
   }
   END{
     pct = (total ? (win_in*100.0/total) : 0);
     printf("period=10ms win=2ms off=%dns\n", off);
     printf("total=%d win_in=%d win_out=%d in_win=%.2f%%\n",
            total, win_in, win_out, pct);
   }'
   period=10ms win=2ms off=0ns
   total=100000 win_in=99977 win_out=23 in_win=99.98%

On Linux host B, change to send VLAN 100 PCP 3 packets every 100us.

.. code-block:: console

   # echo "vlan_p 3" > eth0
   # echo "start" > pgctrl


On Linux host A, capture 100000 packets and check if they fell in 8ms window.
Because gate 3 has 8ms slot, packets can be received only in 8ms window.
The results showed 99.98% were in 8ms window considering some critical condition.

.. code-block:: none

   # tcpdump -i eth0 -j adapter_unsynced --time-stamp-precision nano -tt -e vlan -c 100000 -w pcp2.pcap
   tcpdump: listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
   100000 packets captured
   101793 packets received by filter
   0 packets dropped by kernel
   #
   #
   # tcpdump -r pcp3.pcap --time-stamp-precision nano -tt -n 2>/dev/null | \
   awk '
   BEGIN{
     period=10000000; win=8000000; off=2000000;
     win_in=0; win_out=0; total=0;
   }
   $1 ~ /^[0-9]+\.[0-9]+$/ {
     split($1,a,"."); t=a[1]*1000000000 + (a[2]+0);
     phase = ((t - off) % period + period) % period;
     if (phase < win) win_in++; else win_out++;
     total++;
   }
   END{
     pct = (total ? (win_in*100.0/total) : 0);
     printf("period=10ms win=8ms off=%dns\n", off);
     printf("total=%d win_in=%d win_out=%d in_win=%.2f%%\n",
            total, win_in, win_out, pct);
   }'
   period=10ms win=8ms off=2000000ns
   total=100000 win_in=99976 win_out=24 in_win=99.98%
