.. _traffic-class-support:

Network Traffic Classification
##############################

Overview
********

`Traffic classification <https://en.wikipedia.org/wiki/Traffic_classification>`_
is an automated process that categorizes computer network traffic according to
various parameters. For Zephyr, the VLAN priority code point (PCP) is used
to classify both received and sent network packets. See more information about
VLAN priority at `IEEE 802.1Q <https://en.wikipedia.org/wiki/IEEE_802.1Q>`_.

By default, all network traffic is treated equal in Zephyr. If desired, the
option :option:`CONFIG_NET_TC_TX_COUNT` can be used to set the number of
transmit queues. The option :option:`CONFIG_NET_TC_RX_COUNT` can be used to set
the number of receive queues. Each traffic class queue corresponds to a
specific kernel work queue. Each kernel work queue has a priority.
The VLAN priority is mapped to a certain traffic class according to rules
specified in `IEEE 802.1Q spec`_ chapter I.3, chapter 8.6.6 table 8-4,
and chapter 34.5 table 34-1. Each traffic class is in turn mapped to a certain
kernel work queue. The maximum number of traffic classes for both Rx and Tx
is 8.

See :file:`subsys/net/ip/net_tc.c` for details of how various mappings are done.

.. _IEEE 802.1Q spec: https://ieeexplore.ieee.org/document/6991462/
