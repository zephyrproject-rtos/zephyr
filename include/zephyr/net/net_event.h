/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Network Events code public header
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_EVENT_H_
#define ZEPHYR_INCLUDE_NET_NET_EVENT_H_

#include <zephyr/net/net_ip.h>
#include <zephyr/net/hostname.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup net_mgmt
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Network Interface events */
#define _NET_IF_LAYER		NET_MGMT_LAYER_L2
#define _NET_IF_CORE_CODE	0x001
#define _NET_EVENT_IF_BASE	(NET_MGMT_EVENT_BIT |			\
				 NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_IF_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_IF_CORE_CODE))

enum net_event_if_cmd {
	NET_EVENT_IF_CMD_DOWN = 1,
	NET_EVENT_IF_CMD_UP,
	NET_EVENT_IF_CMD_ADMIN_DOWN,
	NET_EVENT_IF_CMD_ADMIN_UP,
};

/* IPv6 Events */
#define _NET_IPV6_LAYER		NET_MGMT_LAYER_L3
#define _NET_IPV6_CORE_CODE	0x060
#define _NET_EVENT_IPV6_BASE	(NET_MGMT_EVENT_BIT |			\
				 NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_IPV6_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_IPV6_CORE_CODE))

enum net_event_ipv6_cmd {
	NET_EVENT_IPV6_CMD_ADDR_ADD	= 1,
	NET_EVENT_IPV6_CMD_ADDR_DEL,
	NET_EVENT_IPV6_CMD_MADDR_ADD,
	NET_EVENT_IPV6_CMD_MADDR_DEL,
	NET_EVENT_IPV6_CMD_PREFIX_ADD,
	NET_EVENT_IPV6_CMD_PREFIX_DEL,
	NET_EVENT_IPV6_CMD_MCAST_JOIN,
	NET_EVENT_IPV6_CMD_MCAST_LEAVE,
	NET_EVENT_IPV6_CMD_ROUTER_ADD,
	NET_EVENT_IPV6_CMD_ROUTER_DEL,
	NET_EVENT_IPV6_CMD_ROUTE_ADD,
	NET_EVENT_IPV6_CMD_ROUTE_DEL,
	NET_EVENT_IPV6_CMD_DAD_SUCCEED,
	NET_EVENT_IPV6_CMD_DAD_FAILED,
	NET_EVENT_IPV6_CMD_NBR_ADD,
	NET_EVENT_IPV6_CMD_NBR_DEL,
	NET_EVENT_IPV6_CMD_DHCP_START,
	NET_EVENT_IPV6_CMD_DHCP_BOUND,
	NET_EVENT_IPV6_CMD_DHCP_STOP,
	NET_EVENT_IPV6_CMD_ADDR_DEPRECATED,
	NET_EVENT_IPV6_CMD_PE_ENABLED,
	NET_EVENT_IPV6_CMD_PE_DISABLED,
	NET_EVENT_IPV6_CMD_PE_FILTER_ADD,
	NET_EVENT_IPV6_CMD_PE_FILTER_DEL,
	NET_EVENT_IPV6_CMD_PMTU_CHANGED,
};

/* IPv4 Events*/
#define _NET_IPV4_LAYER		NET_MGMT_LAYER_L3
#define _NET_IPV4_CORE_CODE	0x004
#define _NET_EVENT_IPV4_BASE	(NET_MGMT_EVENT_BIT |			\
				 NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_IPV4_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_IPV4_CORE_CODE))

enum net_event_ipv4_cmd {
	NET_EVENT_IPV4_CMD_ADDR_ADD	= 1,
	NET_EVENT_IPV4_CMD_ADDR_DEL,
	NET_EVENT_IPV4_CMD_MADDR_ADD,
	NET_EVENT_IPV4_CMD_MADDR_DEL,
	NET_EVENT_IPV4_CMD_ROUTER_ADD,
	NET_EVENT_IPV4_CMD_ROUTER_DEL,
	NET_EVENT_IPV4_CMD_DHCP_START,
	NET_EVENT_IPV4_CMD_DHCP_BOUND,
	NET_EVENT_IPV4_CMD_DHCP_STOP,
	NET_EVENT_IPV4_CMD_MCAST_JOIN,
	NET_EVENT_IPV4_CMD_MCAST_LEAVE,
	NET_EVENT_IPV4_CMD_ACD_SUCCEED,
	NET_EVENT_IPV4_CMD_ACD_FAILED,
	NET_EVENT_IPV4_CMD_ACD_CONFLICT,
	NET_EVENT_IPV4_CMD_PMTU_CHANGED,
};

/* L4 network events */
#define _NET_L4_LAYER		NET_MGMT_LAYER_L4
#define _NET_L4_CORE_CODE	0x114
#define _NET_EVENT_L4_BASE	(NET_MGMT_EVENT_BIT |			\
				 NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_L4_LAYER) |	\
				 NET_MGMT_LAYER_CODE(_NET_L4_CORE_CODE))

enum net_event_l4_cmd {
	NET_EVENT_L4_CMD_CONNECTED = 1,
	NET_EVENT_L4_CMD_DISCONNECTED,
	NET_EVENT_L4_CMD_IPV4_CONNECTED,
	NET_EVENT_L4_CMD_IPV4_DISCONNECTED,
	NET_EVENT_L4_CMD_IPV6_CONNECTED,
	NET_EVENT_L4_CMD_IPV6_DISCONNECTED,
	NET_EVENT_L4_CMD_DNS_SERVER_ADD,
	NET_EVENT_L4_CMD_DNS_SERVER_DEL,
	NET_EVENT_L4_CMD_HOSTNAME_CHANGED,
	NET_EVENT_L4_CMD_CAPTURE_STARTED,
	NET_EVENT_L4_CMD_CAPTURE_STOPPED,
};

/** @endcond */

/** Event emitted when the network interface goes down. */
#define NET_EVENT_IF_DOWN					\
	(_NET_EVENT_IF_BASE | NET_EVENT_IF_CMD_DOWN)

/** Event emitted when the network interface goes up. */
#define NET_EVENT_IF_UP						\
	(_NET_EVENT_IF_BASE | NET_EVENT_IF_CMD_UP)

/** Event emitted when the network interface is taken down manually. */
#define NET_EVENT_IF_ADMIN_DOWN					\
	(_NET_EVENT_IF_BASE | NET_EVENT_IF_CMD_ADMIN_DOWN)

/** Event emitted when the network interface goes up manually. */
#define NET_EVENT_IF_ADMIN_UP					\
	(_NET_EVENT_IF_BASE | NET_EVENT_IF_CMD_ADMIN_UP)

/** Event emitted when an IPv6 address is added to the system. */
#define NET_EVENT_IPV6_ADDR_ADD					\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ADDR_ADD)

/** Event emitted when an IPv6 address is removed from the system. */
#define NET_EVENT_IPV6_ADDR_DEL					\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_ADDR_DEL)

/** Event emitted when an IPv6 multicast address is added to the system. */
#define NET_EVENT_IPV6_MADDR_ADD				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_MADDR_ADD)

/** Event emitted when an IPv6 multicast address is removed from the system. */
#define NET_EVENT_IPV6_MADDR_DEL				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_MADDR_DEL)

/** Event emitted when an IPv6 prefix is added to the system. */
#define NET_EVENT_IPV6_PREFIX_ADD				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_PREFIX_ADD)

/** Event emitted when an IPv6 prefix is removed from the system. */
#define NET_EVENT_IPV6_PREFIX_DEL				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_PREFIX_DEL)

/** Event emitted when an IPv6 multicast group is joined. */
#define NET_EVENT_IPV6_MCAST_JOIN				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_MCAST_JOIN)

/** Event emitted when an IPv6 multicast group is left. */
#define NET_EVENT_IPV6_MCAST_LEAVE				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_MCAST_LEAVE)

/** Event emitted when an IPv6 router is added to the system. */
#define NET_EVENT_IPV6_ROUTER_ADD				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ROUTER_ADD)

/** Event emitted when an IPv6 router is removed from the system. */
#define NET_EVENT_IPV6_ROUTER_DEL				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ROUTER_DEL)

/** Event emitted when an IPv6 route is added to the system. */
#define NET_EVENT_IPV6_ROUTE_ADD				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ROUTE_ADD)

/** Event emitted when an IPv6 route is removed from the system. */
#define NET_EVENT_IPV6_ROUTE_DEL				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ROUTE_DEL)

/** Event emitted when an IPv6 duplicate address detection succeeds. */
#define NET_EVENT_IPV6_DAD_SUCCEED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_DAD_SUCCEED)

/** Event emitted when an IPv6 duplicate address detection fails. */
#define NET_EVENT_IPV6_DAD_FAILED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_DAD_FAILED)

/** Event emitted when an IPv6 neighbor is added to the system. */
#define NET_EVENT_IPV6_NBR_ADD					\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_NBR_ADD)

/** Event emitted when an IPv6 neighbor is removed from the system. */
#define NET_EVENT_IPV6_NBR_DEL					\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_NBR_DEL)

/** Event emitted when an IPv6 DHCP client starts. */
#define NET_EVENT_IPV6_DHCP_START				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_DHCP_START)

/** Event emitted when an IPv6 DHCP client address is bound. */
#define NET_EVENT_IPV6_DHCP_BOUND				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_DHCP_BOUND)

/** Event emitted when an IPv6 DHCP client is stopped. */
#define NET_EVENT_IPV6_DHCP_STOP				\
	(_NET_EVENT_IPV6_BASE |	NET_EVENT_IPV6_CMD_DHCP_STOP)

/** IPv6 address is deprecated. */
#define NET_EVENT_IPV6_ADDR_DEPRECATED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ADDR_DEPRECATED)

/** IPv6 Privacy extension is enabled. */
#define NET_EVENT_IPV6_PE_ENABLED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_PE_ENABLED)

/** IPv6 Privacy extension is disabled. */
#define NET_EVENT_IPV6_PE_DISABLED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_PE_DISABLED)

/** IPv6 Privacy extension filter is added. */
#define NET_EVENT_IPV6_PE_FILTER_ADD				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_PE_FILTER_ADD)

/** IPv6 Privacy extension filter is removed. */
#define NET_EVENT_IPV6_PE_FILTER_DEL				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_PE_FILTER_DEL)

/** IPv6 Path MTU is changed. */
#define NET_EVENT_IPV6_PMTU_CHANGED				\
	(_NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_PMTU_CHANGED)

/** Event emitted when an IPv4 address is added to the system. */
#define NET_EVENT_IPV4_ADDR_ADD					\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_ADDR_ADD)

/** Event emitted when an IPv4 address is removed from the system. */
#define NET_EVENT_IPV4_ADDR_DEL					\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_ADDR_DEL)

/** Event emitted when an IPv4 multicast address is added to the system. */
#define NET_EVENT_IPV4_MADDR_ADD				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_MADDR_ADD)

/** Event emitted when an IPv4 multicast address is removed from the system. */
#define NET_EVENT_IPV4_MADDR_DEL				\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_MADDR_DEL)

/** Event emitted when an IPv4 router is added to the system. */
#define NET_EVENT_IPV4_ROUTER_ADD				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_ROUTER_ADD)

/** Event emitted when an IPv4 router is removed from the system. */
#define NET_EVENT_IPV4_ROUTER_DEL				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_ROUTER_DEL)

/** Event emitted when an IPv4 DHCP client is started. */
#define NET_EVENT_IPV4_DHCP_START				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_DHCP_START)

/** Event emitted when an IPv4 DHCP client address is bound. */
#define NET_EVENT_IPV4_DHCP_BOUND				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_DHCP_BOUND)

/** Event emitted when an IPv4 DHCP client is stopped. */
#define NET_EVENT_IPV4_DHCP_STOP				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_DHCP_STOP)

/** Event emitted when an IPv4 multicast group is joined. */
#define NET_EVENT_IPV4_MCAST_JOIN				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_MCAST_JOIN)

/** Event emitted when an IPv4 multicast group is left. */
#define NET_EVENT_IPV4_MCAST_LEAVE				\
	(_NET_EVENT_IPV4_BASE |	NET_EVENT_IPV4_CMD_MCAST_LEAVE)

/** Event emitted when an IPv4 address conflict detection succeeds. */
#define NET_EVENT_IPV4_ACD_SUCCEED				\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_ACD_SUCCEED)

/** Event emitted when an IPv4 address conflict detection fails. */
#define NET_EVENT_IPV4_ACD_FAILED				\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_ACD_FAILED)

/** Event emitted when an IPv4 address conflict was detected after the address
 *  was confirmed as safe to use. It's up to the application to determine on
 *  how to act in such case.
 */
#define NET_EVENT_IPV4_ACD_CONFLICT				\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_ACD_CONFLICT)

/** IPv4 Path MTU is changed. */
#define NET_EVENT_IPV4_PMTU_CHANGED				\
	(_NET_EVENT_IPV4_BASE | NET_EVENT_IPV4_CMD_PMTU_CHANGED)

/** Event emitted when the system is considered to be connected.
 * The connected in this context means that the network interface is up,
 * and the interface has either IPv4 or IPv6 address assigned to it.
 */
#define NET_EVENT_L4_CONNECTED					\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_CONNECTED)

/** Event emitted when the system is no longer connected.
 * Typically this means that network connectivity is lost either by
 * the network interface is going down, or the interface has no longer
 * an IP address etc.
 */
#define NET_EVENT_L4_DISCONNECTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_DISCONNECTED)


/** Event raised when IPv4 network connectivity is available. */
#define NET_EVENT_L4_IPV4_CONNECTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_IPV4_CONNECTED)

/** Event emitted when IPv4 network connectivity is lost. */
#define NET_EVENT_L4_IPV4_DISCONNECTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_IPV4_DISCONNECTED)

/** Event emitted when IPv6 network connectivity is available. */
#define NET_EVENT_L4_IPV6_CONNECTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_IPV6_CONNECTED)

/** Event emitted when IPv6 network connectivity is lost. */
#define NET_EVENT_L4_IPV6_DISCONNECTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_IPV6_DISCONNECTED)

/** Event emitted when a DNS server is added to the system. */
#define NET_EVENT_DNS_SERVER_ADD			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_DNS_SERVER_ADD)

/** Event emitted when a DNS server is removed from the system. */
#define NET_EVENT_DNS_SERVER_DEL			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_DNS_SERVER_DEL)

/** Event emitted when the system hostname is changed. */
#define NET_EVENT_HOSTNAME_CHANGED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_HOSTNAME_CHANGED)

/** Network packet capture is started. */
#define NET_EVENT_CAPTURE_STARTED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_CAPTURE_STARTED)

/** Network packet capture is stopped. */
#define NET_EVENT_CAPTURE_STOPPED			\
	(_NET_EVENT_L4_BASE | NET_EVENT_L4_CMD_CAPTURE_STOPPED)

/**
 * @brief Network Management event information structure
 * Used to pass information on network events like
 *   NET_EVENT_IPV6_ADDR_ADD,
 *   NET_EVENT_IPV6_ADDR_DEL,
 *   NET_EVENT_IPV6_MADDR_ADD and
 *   NET_EVENT_IPV6_MADDR_DEL
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 */
struct net_event_ipv6_addr {
	/** IPv6 address related to this event */
	struct in6_addr addr;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network events like
 *   NET_EVENT_IPV6_NBR_ADD and
 *   NET_EVENT_IPV6_NBR_DEL
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 * @note: idx will be '-1' in case of NET_EVENT_IPV6_NBR_DEL event.
 */
struct net_event_ipv6_nbr {
	/** Neighbor IPv6 address */
	struct in6_addr addr;
	/** Neighbor index in cache */
	int idx;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network events like
 *   NET_EVENT_IPV6_ROUTE_ADD and
 *   NET_EVENT_IPV6_ROUTE_DEL
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 */
struct net_event_ipv6_route {
	/** IPv6 address of the next hop */
	struct in6_addr nexthop;
	/** IPv6 address or prefix of the route */
	struct in6_addr addr;
	/** IPv6 prefix length */
	uint8_t prefix_len;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network events like
 *   NET_EVENT_IPV6_PREFIX_ADD and
 *   NET_EVENT_IPV6_PREFIX_DEL
 * when CONFIG_NET_MGMT_EVENT_INFO is enabled and event generator pass the
 * information.
 */
struct net_event_ipv6_prefix {
	/** IPv6 prefix */
	struct in6_addr addr;
	/** IPv6 prefix length */
	uint8_t len;
	/** IPv6 prefix lifetime in seconds */
	uint32_t lifetime;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on NET_EVENT_HOSTNAME_CHANGED event when
 * CONFIG_NET_MGMT_EVENT_INFO is enabled and event generator pass the
 * information.
 */
struct net_event_l4_hostname {
	/** New hostname */
	char hostname[NET_HOSTNAME_SIZE];
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network events like
 *   NET_EVENT_IPV6_PE_FILTER_ADD and
 *   NET_EVENT_IPV6_PE_FILTER_DEL
 * when CONFIG_NET_MGMT_EVENT_INFO is enabled and event generator pass the
 * information.
 *
 * This is only available if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT is >0.
 */
struct net_event_ipv6_pe_filter {
	/** IPv6 address of privacy extension filter */
	struct in6_addr prefix;
	/** IPv6 filter deny or allow list */
	bool is_deny_list;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network event
 *   NET_EVENT_IPV4_PMTU_CHANGED
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 */
struct net_event_ipv4_pmtu_info {
	/** IPv4 address */
	struct in_addr dst;
	/** New MTU */
	uint16_t mtu;
};

/**
 * @brief Network Management event information structure
 * Used to pass information on network event
 *   NET_EVENT_IPV6_PMTU_CHANGED
 * when CONFIG_NET_MGMT_EVENT_INFO enabled and event generator pass the
 * information.
 */
struct net_event_ipv6_pmtu_info {
	/** IPv6 address */
	struct in6_addr dst;
	/** New MTU */
	uint32_t mtu;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_EVENT_H_ */
