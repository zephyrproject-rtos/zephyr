/** @file
 * @brief Network packet filtering public header file
 *
 * The network packet filtering provides a mechanism for deciding the fate
 * of an incoming or outgoing packet based on a set of basic rules.
 */

/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_PKT_FILTER_H_
#define ZEPHYR_INCLUDE_NET_PKT_FILTER_H_

#include <limits.h>
#include <stdbool.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Packet Filter API
 * @defgroup net_pkt_filter Network Packet Filter API
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct npf_test;

typedef bool (npf_test_fn_t)(struct npf_test *test, struct net_pkt *pkt);

/** @endcond */

/** @brief common filter test structure to be embedded into larger structures */
struct npf_test {
	npf_test_fn_t *fn;	/**< packet condition test function */
};

/** @brief filter rule structure */
struct npf_rule {
	sys_snode_t node;               /**< Slist rule list node */
	enum net_verdict result;	/**< result if all tests pass */
	uint32_t nb_tests;		/**< number of tests for this rule */
	struct npf_test *tests[];	/**< pointers to @ref npf_test instances */
};

/** @brief Default rule list termination for accepting a packet */
extern struct npf_rule npf_default_ok;
/** @brief Default rule list termination for rejecting a packet */
extern struct npf_rule npf_default_drop;

/** @brief rule set for a given test location */
struct npf_rule_list {
	sys_slist_t rule_head;   /**< List head */
	struct k_spinlock lock;  /**< Lock protecting the list access */
};

/** @brief  rule list applied to outgoing packets */
extern struct npf_rule_list npf_send_rules;
/** @brief rule list applied to incoming packets */
extern struct npf_rule_list npf_recv_rules;
/** @brief rule list applied for local incoming packets */
extern struct npf_rule_list npf_local_in_recv_rules;
/** @brief rule list applied for IPv4 incoming packets */
extern struct npf_rule_list npf_ipv4_recv_rules;
/** @brief rule list applied for IPv6 incoming packets */
extern struct npf_rule_list npf_ipv6_recv_rules;

/**
 * @brief Insert a rule at the front of given rule list
 *
 * @param rules the affected rule list
 * @param rule the rule to be inserted
 */
void npf_insert_rule(struct npf_rule_list *rules, struct npf_rule *rule);

/**
 * @brief Append a rule at the end of given rule list
 *
 * @param rules the affected rule list
 * @param rule the rule to be appended
 */
void npf_append_rule(struct npf_rule_list *rules, struct npf_rule *rule);

/**
 * @brief Remove a rule from the given rule list
 *
 * @param rules the affected rule list
 * @param rule the rule to be removed
 * @retval true if given rule was found in the rule list and removed
 */
bool npf_remove_rule(struct npf_rule_list *rules, struct npf_rule *rule);

/**
 * @brief Remove all rules from the given rule list
 *
 * @param rules the affected rule list
 * @retval true if at least one rule was removed from the rule list
 */
bool npf_remove_all_rules(struct npf_rule_list *rules);

/** @cond INTERNAL_HIDDEN */

/* convenience shortcuts */
#define npf_insert_send_rule(rule) npf_insert_rule(&npf_send_rules, rule)
#define npf_insert_recv_rule(rule) npf_insert_rule(&npf_recv_rules, rule)
#define npf_append_send_rule(rule) npf_append_rule(&npf_send_rules, rule)
#define npf_append_recv_rule(rule) npf_append_rule(&npf_recv_rules, rule)
#define npf_remove_send_rule(rule) npf_remove_rule(&npf_send_rules, rule)
#define npf_remove_recv_rule(rule) npf_remove_rule(&npf_recv_rules, rule)
#define npf_remove_all_send_rules() npf_remove_all_rules(&npf_send_rules)
#define npf_remove_all_recv_rules() npf_remove_all_rules(&npf_recv_rules)

#ifdef CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK
#define npf_insert_local_in_recv_rule(rule) npf_insert_rule(&npf_local_in_recv_rules, rule)
#define npf_append_local_in_recv_rule(rule) npf_append_rule(&npf_local_in_recv_rules, rule)
#define npf_remove_local_in_recv_rule(rule) npf_remove_rule(&npf_local_in_recv_rules, rule)
#define npf_remove_all_local_in_recv_rules() npf_remove_all_rules(&npf_local_in_recv_rules)
#endif /* CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK */

#ifdef CONFIG_NET_PKT_FILTER_IPV4_HOOK
#define npf_insert_ipv4_recv_rule(rule) npf_insert_rule(&npf_ipv4_recv_rules, rule)
#define npf_append_ipv4_recv_rule(rule) npf_append_rule(&npf_ipv4_recv_rules, rule)
#define npf_remove_ipv4_recv_rule(rule) npf_remove_rule(&npf_ipv4_recv_rules, rule)
#define npf_remove_all_ipv4_recv_rules() npf_remove_all_rules(&npf_ipv4_recv_rules)
#endif /* CONFIG_NET_PKT_FILTER_IPV4_HOOK */

#ifdef CONFIG_NET_PKT_FILTER_IPV6_HOOK
#define npf_insert_ipv6_recv_rule(rule) npf_insert_rule(&npf_ipv6_recv_rules, rule)
#define npf_append_ipv6_recv_rule(rule) npf_append_rule(&npf_ipv6_recv_rules, rule)
#define npf_remove_ipv6_recv_rule(rule) npf_remove_rule(&npf_ipv6_recv_rules, rule)
#define npf_remove_all_ipv6_recv_rules() npf_remove_all_rules(&npf_ipv6_recv_rules)
#endif /* CONFIG_NET_PKT_FILTER_IPV6_HOOK */

/** @endcond */

/**
 * @brief Statically define one packet filter rule
 *
 * This creates a rule from a variable amount of filter conditions.
 * This rule can then be inserted or appended to the rule list for a given
 * network packet path.
 *
 * Example:
 *
 * @code{.c}
 *
 *     static NPF_SIZE_MAX(maxsize_200, 200);
 *     static NPF_ETH_TYPE_MATCH(ip_packet, NET_ETH_PTYPE_IP);
 *
 *     static NPF_RULE(small_ip_pkt, NET_OK, ip_packet, maxsize_200);
 *
 *     void install_my_filter(void)
 *     {
 *         npf_insert_recv_rule(&npf_default_drop);
 *         npf_insert_recv_rule(&small_ip_pkt);
 *     }
 *
 * @endcode
 *
 * The above would accept IP packets that are 200 bytes or smaller, and drop
 * all other packets.
 *
 * Another (less efficient) way to create the same result could be:
 *
 * @code{.c}
 *
 *     static NPF_SIZE_MIN(minsize_201, 201);
 *     static NPF_ETH_TYPE_UNMATCH(not_ip_packet, NET_ETH_PTYPE_IP);
 *
 *     static NPF_RULE(reject_big_pkts, NET_DROP, minsize_201);
 *     static NPF_RULE(reject_non_ip, NET_DROP, not_ip_packet);
 *
 *     void install_my_filter(void) {
 *         npf_append_recv_rule(&reject_big_pkts);
 *         npf_append_recv_rule(&reject_non_ip);
 *         npf_append_recv_rule(&npf_default_ok);
 *     }
 *
 * @endcode
 *
 * The first rule in the list for which all conditions are true determines
 * the fate of the packet. If one condition is false then the next rule in
 * the list is evaluated.
 *
 * @param _name Name for this rule.
 * @param _result Fate of the packet if all conditions are true, either
 *                <tt>NET_OK</tt> or <tt>NET_DROP</tt>.
 * @param ... List of conditions for this rule.
 */
#define NPF_RULE(_name, _result, ...) \
	struct npf_rule _name = { \
		.result = (_result), \
		.nb_tests = NUM_VA_ARGS_LESS_1(__VA_ARGS__) + 1, \
		.tests = { FOR_EACH(Z_NPF_TEST_ADDR, (,), __VA_ARGS__) }, \
	}

#define Z_NPF_TEST_ADDR(arg) &arg.test

/** @} */

/**
 * @defgroup npf_basic_cond Basic Filter Conditions
 * @ingroup net_pkt_filter
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct npf_test_iface {
	struct npf_test test;
	struct net_if *iface;
};

extern npf_test_fn_t npf_iface_match;
extern npf_test_fn_t npf_iface_unmatch;
extern npf_test_fn_t npf_orig_iface_match;
extern npf_test_fn_t npf_orig_iface_unmatch;

/** @endcond */

/**
 * @brief Statically define an "interface match" packet filter condition
 *
 * @param _name Name of the condition
 * @param _iface Interface to match
 */
#define NPF_IFACE_MATCH(_name, _iface) \
	struct npf_test_iface _name = { \
		.iface = (_iface), \
		.test.fn = npf_iface_match, \
	}

/**
 * @brief Statically define an "interface unmatch" packet filter condition
 *
 * @param _name Name of the condition
 * @param _iface Interface to exclude
 */
#define NPF_IFACE_UNMATCH(_name, _iface) \
	struct npf_test_iface _name = { \
		.iface = (_iface), \
		.test.fn = npf_iface_unmatch, \
	}

/**
 * @brief Statically define an "orig interface match" packet filter condition
 *
 * @param _name Name of the condition
 * @param _iface Interface to match
 */
#define NPF_ORIG_IFACE_MATCH(_name, _iface) \
	struct npf_test_iface _name = { \
		.iface = (_iface), \
		.test.fn = npf_orig_iface_match, \
	}

/**
 * @brief Statically define an "orig interface unmatch" packet filter condition
 *
 * @param _name Name of the condition
 * @param _iface Interface to exclude
 */
#define NPF_ORIG_IFACE_UNMATCH(_name, _iface) \
	struct npf_test_iface _name = { \
		.iface = (_iface), \
		.test.fn = npf_orig_iface_unmatch, \
	}

/** @cond INTERNAL_HIDDEN */

struct npf_test_size_bounds {
	struct npf_test test;
	size_t min;
	size_t max;
};

extern npf_test_fn_t npf_size_inbounds;

/** @endcond */

/**
 * @brief Statically define a "data minimum size" packet filter condition
 *
 * @param _name Name of the condition
 * @param _size Lower bound of the packet's data size
 */
#define NPF_SIZE_MIN(_name, _size) \
	struct npf_test_size_bounds _name = { \
		.min = (_size), \
		.max = SIZE_MAX, \
		.test.fn = npf_size_inbounds, \
	}

/**
 * @brief Statically define a "data maximum size" packet filter condition
 *
 * @param _name Name of the condition
 * @param _size Higher bound of the packet's data size
 */
#define NPF_SIZE_MAX(_name, _size) \
	struct npf_test_size_bounds _name = { \
		.min = 0, \
		.max = (_size), \
		.test.fn = npf_size_inbounds, \
	}

/**
 * @brief Statically define a "data bounded size" packet filter condition
 *
 * @param _name Name of the condition
 * @param _min_size Lower bound of the packet's data size
 * @param _max_size Higher bound of the packet's data size
 */
#define NPF_SIZE_BOUNDS(_name, _min_size, _max_size) \
	struct npf_test_size_bounds _name = { \
		.min = (_min_size), \
		.max = (_max_size), \
		.test.fn = npf_size_inbounds, \
	}

/** @cond INTERNAL_HIDDEN */

struct npf_test_ip {
	struct npf_test test;
	uint8_t addr_family;
	void *ipaddr;
	uint32_t ipaddr_num;
};

extern npf_test_fn_t npf_ip_src_addr_match;
extern npf_test_fn_t npf_ip_src_addr_unmatch;

/** @endcond */

/**
 * @brief Statically define a "ip address allowlist" packet filter condition
 *
 * This tests if the packet source ip address matches any of the ip
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _ip_addr_array Array of <tt>struct in_addr</tt> or <tt>struct in6_addr</tt> items to test
 *against
 * @param _ip_addr_num number of IP addresses in the array
 * @param _af Addresses family type (AF_INET / AF_INET6) in the array
 */
#define NPF_IP_SRC_ADDR_ALLOWLIST(_name, _ip_addr_array, _ip_addr_num, _af) \
	struct npf_test_ip _name = { \
		.addr_family = _af, \
		.ipaddr = (_ip_addr_array), \
		.ipaddr_num = _ip_addr_num, \
		.test.fn = npf_ip_src_addr_match, \
	}

/**
 * @brief Statically define a "ip address blocklist" packet filter condition
 *
 * This tests if the packet source ip address matches any of the ip
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _ip_addr_array Array of <tt>struct in_addr</tt> or <tt>struct in6_addr</tt> items to test
 *against
 * @param _ip_addr_num number of IP addresses in the array
 * @param _af Addresses family type (AF_INET / AF_INET6) in the array
 */
#define NPF_IP_SRC_ADDR_BLOCKLIST(_name, _ip_addr_array, _ip_addr_num, _af) \
	struct npf_test_ip _name = { \
		.addr_family = _af, \
		.ipaddr = (_ip_addr_array), \
		.ipaddr_num = _ip_addr_num, \
		.test.fn = npf_ip_src_addr_unmatch, \
	}

/** @} */

/**
 * @defgroup npf_eth_cond Ethernet Filter Conditions
 * @ingroup net_pkt_filter
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct npf_test_eth_addr {
	struct npf_test test;
	unsigned int nb_addresses;
	struct net_eth_addr *addresses;
	struct net_eth_addr mask;
};

extern npf_test_fn_t npf_eth_src_addr_match;
extern npf_test_fn_t npf_eth_src_addr_unmatch;
extern npf_test_fn_t npf_eth_dst_addr_match;
extern npf_test_fn_t npf_eth_dst_addr_unmatch;

/** @endcond */

/**
 * @brief Statically define a "source address match" packet filter condition
 *
 * This tests if the packet source address matches any of the Ethernet
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 */
#define NPF_ETH_SRC_ADDR_MATCH(_name, _addr_array) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.test.fn = npf_eth_src_addr_match, \
		.mask.addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, \
	}

/**
 * @brief Statically define a "source address unmatch" packet filter condition
 *
 * This tests if the packet source address matches none of the Ethernet
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 */
#define NPF_ETH_SRC_ADDR_UNMATCH(_name, _addr_array) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.test.fn = npf_eth_src_addr_unmatch, \
		.mask.addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, \
	}

/**
 * @brief Statically define a "destination address match" packet filter condition
 *
 * This tests if the packet destination address matches any of the Ethernet
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 */
#define NPF_ETH_DST_ADDR_MATCH(_name, _addr_array) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.test.fn = npf_eth_dst_addr_match, \
		.mask.addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, \
	}

/**
 * @brief Statically define a "destination address unmatch" packet filter condition
 *
 * This tests if the packet destination address matches none of the Ethernet
 * addresses contained in the provided set.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 */
#define NPF_ETH_DST_ADDR_UNMATCH(_name, _addr_array) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.test.fn = npf_eth_dst_addr_unmatch, \
		.mask.addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, \
	}

/**
 * @brief Statically define a "source address match with mask" packet filter condition
 *
 * This tests if the packet source address matches any of the Ethernet
 * addresses contained in the provided set after applying specified mask.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 * @param ... up to 6 mask bytes
 */
#define NPF_ETH_SRC_ADDR_MASK_MATCH(_name, _addr_array, ...) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.mask.addr = { __VA_ARGS__ }, \
		.test.fn = npf_eth_src_addr_match, \
	}

/**
 * @brief Statically define a "destination address match with mask" packet filter condition
 *
 * This tests if the packet destination address matches any of the Ethernet
 * addresses contained in the provided set after applying specified mask.
 *
 * @param _name Name of the condition
 * @param _addr_array Array of <tt>struct net_eth_addr</tt> items to test against
 * @param ... up to 6 mask bytes
 */
#define NPF_ETH_DST_ADDR_MASK_MATCH(_name, _addr_array, ...) \
	struct npf_test_eth_addr _name = { \
		.addresses = (_addr_array), \
		.nb_addresses = ARRAY_SIZE(_addr_array), \
		.mask.addr = { __VA_ARGS__ }, \
		.test.fn = npf_eth_dst_addr_match, \
	}

/** @cond INTERNAL_HIDDEN */

struct npf_test_eth_type {
	struct npf_test test;
	uint16_t type;			/* type in network order */
};

extern npf_test_fn_t npf_eth_type_match;
extern npf_test_fn_t npf_eth_type_unmatch;

/** @endcond */

/**
 * @brief Statically define an "Ethernet type match" packet filter condition
 *
 * @param _name Name of the condition
 * @param _type Ethernet type to match
 */
#define NPF_ETH_TYPE_MATCH(_name, _type) \
	struct npf_test_eth_type _name = { \
		.type = htons(_type), \
		.test.fn = npf_eth_type_match, \
	}

/**
 * @brief Statically define an "Ethernet type unmatch" packet filter condition
 *
 * @param _name Name of the condition
 * @param _type Ethernet type to exclude
 */
#define NPF_ETH_TYPE_UNMATCH(_name, _type) \
	struct npf_test_eth_type _name = { \
		.type = htons(_type), \
		.test.fn = npf_eth_type_unmatch, \
	}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_PKT_FILTER_H_ */
