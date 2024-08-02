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
and reception. This can be used to create a basic firewall, control
network traffic, etc.

The :kconfig:option:`CONFIG_NET_PKT_FILTER` must be set in order to enable the
relevant APIs.

Both the transmission and reception paths may have a list of filter rules.
Each rule is made of a set of conditions and a packet outcome. Every packet
is subjected to the conditions attached to a rule. When all the conditions
for a given rule are true then the packet outcome is immediately determined
as specified by the current rule and no more rules are considered. If one
condition is false then the next rule in the list is considered.

Packet outcome is either ``NET_OK`` to accept the packet or ``NET_DROP`` to
drop it.

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
:c:macro:`NPF_RULE()` to create a rule instance to tie them.

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

API Reference
*************

.. doxygengroup:: net_pkt_filter

.. doxygengroup:: npf_basic_cond

.. doxygengroup:: npf_eth_cond
