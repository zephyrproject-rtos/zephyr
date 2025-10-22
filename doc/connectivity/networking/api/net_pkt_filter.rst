.. _net_pkt_filter_interface:

Network Packet Filtering
########################

.. contents::
    :local:
    :depth: 2

Overview
********

The Network Packet Filtering facility provides the infrastructure to
construct custom rules for accepting and/or denying packet transmission
and reception. It also allows to modify the priority of incoming
network packets. This can be used to create a basic firewall, control network
traffic, etc.

The :kconfig:option:`CONFIG_NET_PKT_FILTER` must be set in order to enable the
relevant APIs.

Both the transmission and reception paths may have a list of filter rules.
Each rule is made of a set of conditions and a packet outcome. Every packet
is subjected to the conditions attached to a rule. When all the conditions
for a given rule are true then the packet outcome is immediately determined
as specified by the current rule and no more rules are considered. If one
condition is false then the next rule in the list is considered.

Packet outcome is either ``NET_OK`` to accept the packet, ``NET_DROP`` to
drop it or ``NET_CONTINUE`` to modify its priority on the fly.

When the outcome is ``NET_CONTINUE`` the priority is updated but the final
outcome is not yet determined and processing continues. If all conditions of
multiple rules are true, then the packet gets the priority of the rule last
considered.

A rule is represented by a :c:struct:`npf_rule` object. It can be inserted to,
appended to or removed from a rule list contained in a
:c:struct:`npf_rule_list` object using :c:func:`npf_insert_rule()`,
:c:func:`npf_append_rule()`, and :c:func:`npf_remove_rule()`.
Currently, two such rule lists exist: ``npf_send_rules`` for outgoing packets,
and ``npf_recv_rules`` for incoming packets.

If a filter rule list is empty then ``NET_OK`` is assumed. If a non-empty
rule list runs to the end then ``NET_DROP`` is assumed. However it is
recommended to always terminate a non-empty rule list with an explicit
default termination rule, either ``npf_default_ok`` or ``npf_default_drop``.

Rule conditions are represented by a :c:struct:`npf_test`. This structure
can be embedded into a larger structure when a specific condition requires
extra test data. It is up to the test function for such conditions to
retrieve the outer structure from the provided ``npf_test`` structure pointer.

Convenience macros are provided in :zephyr_file:`include/zephyr/net/net_pkt_filter.h`
to statically define condition instances for various conditions, and
:c:macro:`NPF_RULE()` and :c:macro:`NPF_PRIORITY()` to create a rule instance
with an immediate outcome or a priority change.

Examples
********

Here's an example usage:

.. code-block:: c

    static NPF_SIZE_MAX(maxsize_200, 200);
    static NPF_ETH_TYPE_MATCH(ip_packet, NET_ETH_PTYPE_IP);

    static NPF_RULE(small_ip_pkt, NET_OK, ip_packet, maxsize_200);

    void install_my_filter(void)
    {
        npf_insert_recv_rule(&npf_default_drop);
        npf_insert_recv_rule(&small_ip_pkt);
    }

The above would accept IP packets that are 200 bytes or smaller, and drop
all other packets.

Another (less efficient) way to achieve the same result could be:

.. code-block:: c

    static NPF_SIZE_MIN(minsize_201, 201);
    static NPF_ETH_TYPE_UNMATCH(not_ip_packet, NET_ETH_PTYPE_IP);

    static NPF_RULE(reject_big_pkts, NET_DROP, minsize_201);
    static NPF_RULE(reject_non_ip, NET_DROP, not_ip_packet);

    void install_my_filter(void) {
        npf_append_recv_rule(&reject_big_pkts);
        npf_append_recv_rule(&reject_non_ip);
        npf_append_recv_rule(&npf_default_ok);
    }

This example assigns priorities to different network traffic. It gives network
control priority (``NET_PRIORITY_NC``) to the ``ptp`` packets, critical
applications priority (``NET_PRIORITY_CA``) to the internet traffic of version
6, excellent effort (``NET_PRIORITY_EE``) for internet protocol version 4
traffic, and the lowest background priority (``NET_PRIORITY_BK``) to ``lldp``
and ``arp``.

Priority rules are only really useful if multiple traffic class queues are
enabled in the project configuration :kconfig:option:`CONFIG_NET_TC_RX_COUNT`.
The mapping from the priority of the packet to the traffic class queue is in
accordance with the standard 802.1Q and depends on the
:kconfig:option:`CONFIG_NET_TC_RX_COUNT`.

.. code-block:: c

    static NPF_ETH_TYPE_MATCH(is_arp, NET_ETH_PTYPE_ARP);
    static NPF_ETH_TYPE_MATCH(is_lldp, NET_ETH_PTYPE_LLDP);
    static NPF_ETH_TYPE_MATCH(is_ptp, NET_ETH_PTYPE_PTP);
    static NPF_ETH_TYPE_MATCH(is_ipv4, NET_ETH_PTYPE_IP);
    static NPF_ETH_TYPE_MATCH(is_ipv6, NET_ETH_PTYPE_IPV6);

    static NPF_PRIORITY(priority_bk, NET_PRIORITY_BK, is_arp, is_lldp);
    static NPF_PRIORITY(priority_ee, NET_PRIORITY_EE, is_ipv4);
    static NPF_PRIORITY(priority_ca, NET_PRIORITY_CA, is_ipv6);
    static NPF_PRIORITY(priority_nc, NET_PRIORITY_NC, is_ptp);

    void install_my_filter(void) {
        npf_append_recv_rule(&priority_bk);
        npf_append_recv_rule(&priority_ee);
        npf_append_recv_rule(&priority_ca);
        npf_append_recv_rule(&priority_nc);
        npf_append_recv_rule(&npf_default_ok);
    }

API Reference
*************

.. doxygengroup:: net_pkt_filter

.. doxygengroup:: npf_basic_cond

.. doxygengroup:: npf_eth_cond
