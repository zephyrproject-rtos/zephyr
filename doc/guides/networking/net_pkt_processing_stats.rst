.. _net_pkt_processing_stats:

Network Packet Processing Statistics
####################################

.. contents::
    :local:
    :depth: 2

This page describes how to get information about network packet processing
statistics inside network stack.

Network stack contains infrastructure to figure out how long the network packet
processing takes either in sending or receiving path. There are two Kconfig
options that control this. For transmit (TX) path the option is called
:option:`CONFIG_NET_PKT_TXTIME_STATS` and for receive (RX) path the options is
called :option:`CONFIG_NET_PKT_RXTIME_STATS`. Note that for TX, all kind of
network packet statistics is collected. For RX, only UDP, TCP or raw packet
type network packet statistics is collected.

After enabling these options, the :ref:`net stats <net_shell>` network shell
command will show this information:

.. code-block:: console

   Avg TX net_pkt (11484) time 67 us
   Avg RX net_pkt (11474) time 43 us

.. note::

   The values above and below are from emulated qemu_x86 board and UDP traffic

The TX time tells how long it took for network packet from its creation to
when it was sent to the network. The RX time tells the time from its creation
to when it was passed to the application. The values are in microseconds. The
statistics will be collected per traffic class if there are more than one
transmit or receive queues defined in the system. These are controlled by
:option:`CONFIG_NET_TC_TX_COUNT` and :option:`CONFIG_NET_TC_RX_COUNT` options.

If you enable :option:`CONFIG_NET_PKT_TXTIME_STATS_DETAIL` or
:option:`CONFIG_NET_PKT_RXTIME_STATS_DETAIL` options, then additional
information for TX or RX network packets are collected when the network packet
traverses the IP stack.

After enabling these options, the :ref:`net stats <net_shell>` will show
this information:

.. code-block:: console

   Avg TX net_pkt (18902) time 63 us    [0->22->15->23=60 us]
   Avg RX net_pkt (18892) time 42 us    [0->9->6->11->13=39 us]

The numbers inside the brackets contain information how many microseconds it
took for a network packet to go from previous state to next.

In the TX example above, the values are averages over **18902** packets and
contain this information:

* Packet was created by application so the time is **0**.
* Packet is about to be placed to transmit queue. The time it took from network
  packet creation to this state, is **22** microseconds in this example.
* The correct TX thread is invoked, and the packet is read from the transmit
  queue. It took **15** microseconds from previous state.
* The network packet was just sent and the network stack is about to free the
  network packet. It took **23** microseconds from previous state.
* In total it took on average **60** microseconds to get the network packet
  sent. The value **63** tells also the same information, but is calculated
  differently so there is slight difference because of rounding errors.

In the RX example above, the values are averages over **18892** packets and
contain this information:

* Packet was created network device driver so the time is **0**.
* Packet is about to be placed to receive queue. The time it took from network
  packet creation to this state, is **9** microseconds in this example.
* The correct RX thread is invoked, and the packet is read from the receive
  queue. It took **6** microseconds from previous state.
* The network packet is then processed and placed to correct socket queue.
  It took **11** microseconds from previous state.
* The last value tells how long it took from there to the application. Here
  the value is **13** microseconds.
* In total it took on average **39** microseconds to get the network packet
  sent. The value **42** tells also the same information, but is calculated
  differently so there is slight difference because of rounding errors.
