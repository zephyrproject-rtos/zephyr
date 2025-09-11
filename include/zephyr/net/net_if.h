/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for network interface
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_IF_H_
#define ZEPHYR_INCLUDE_NET_NET_IF_H_

/**
 * @brief Network Interface abstraction layer
 * @defgroup net_if Network Interface abstraction layer
 * @since 1.5
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_timeout.h>

#if defined(CONFIG_NET_DHCPV4) && defined(CONFIG_NET_NATIVE_IPV4)
#include <zephyr/net/dhcpv4.h>
#endif
#if defined(CONFIG_NET_DHCPV6) && defined(CONFIG_NET_NATIVE_IPV6)
#include <zephyr/net/dhcpv6.h>
#endif
#if defined(CONFIG_NET_IPV4_AUTO) && defined(CONFIG_NET_NATIVE_IPV4)
#include <zephyr/net/ipv4_autoconf.h>
#endif

#include <zephyr/net/prometheus/collector.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Interface unicast IP addresses
 *
 * Stores the unicast IP addresses assigned to this network interface.
 */
struct net_if_addr {
	/** IP address */
	struct net_addr address;

	/** Reference counter. This is used to prevent address removal if there
	 * are sockets that have bound the local endpoint to this address.
	 */
	atomic_t atomic_ref;

#if defined(CONFIG_NET_NATIVE_IPV6)
	struct net_timeout lifetime;
#endif

	/** How the IP address was set */
	enum net_addr_type addr_type;

	/** What is the current state of the address */
	enum net_addr_state addr_state;

#if defined(CONFIG_NET_NATIVE_IPV6)
#if defined(CONFIG_NET_IPV6_PE)
	/** Address creation time. This is used to determine if the maximum
	 * lifetime for this address is reached or not. The value is in seconds.
	 */
	uint32_t addr_create_time;

	/** Preferred lifetime for the address in seconds.
	 */
	uint32_t addr_preferred_lifetime;

	/** Address timeout value. This is only used if DAD needs to be redone
	 * for this address because of earlier DAD failure. This value is in
	 * seconds.
	 */
	int32_t addr_timeout;
#endif
#endif /* CONFIG_NET_NATIVE_IPV6 */

	union {
#if defined(CONFIG_NET_IPV6_DAD)
		struct {
			/** Duplicate address detection (DAD) timer */
			sys_snode_t dad_node;

			/** DAD needed list node */
			sys_snode_t dad_need_node;

			/** DAD start time */
			uint32_t dad_start;

			/** How many times we have done DAD */
			uint8_t dad_count;
		};
#endif /* CONFIG_NET_IPV6_DAD */
#if defined(CONFIG_NET_IPV4_ACD)
		struct {
			/** Address conflict detection (ACD) timer. */
			sys_snode_t acd_node;

			/** ACD needed list node */
			sys_snode_t acd_need_node;

			/** ACD timeout value. */
			k_timepoint_t acd_timeout;

			/** ACD probe/announcement counter. */
			uint8_t acd_count;

			/** ACD status. */
			uint8_t acd_state;
		};
#endif /* CONFIG_NET_IPV4_ACD */
		uint8_t _dummy;
	};

#if defined(CONFIG_NET_IPV6_DAD) || defined(CONFIG_NET_IPV4_ACD)
	/** What interface the conflict detection is running */
	uint8_t ifindex;
#endif

	/** Is the IP address valid forever */
	uint8_t is_infinite : 1;

	/** Is this IP address used or not */
	uint8_t is_used : 1;

	/** Is this IP address usage limited to the subnet (mesh) or not */
	uint8_t is_mesh_local : 1;

	/** Is this IP address temporary and generated for example by
	 * IPv6 privacy extension (RFC 8981)
	 */
	uint8_t is_temporary : 1;

	/** Was this address added or not */
	uint8_t is_added : 1;

	uint8_t _unused : 3;
};

/**
 * @brief Network Interface multicast IP addresses
 *
 * Stores the multicast IP addresses assigned to this network interface.
 */
struct net_if_mcast_addr {
	/** IP address */
	struct net_addr address;

	/** Rejoining multicast groups list node */
	sys_snode_t rejoin_node;

#if defined(CONFIG_NET_IPV4_IGMPV3)
	/** Sources to filter on */
	struct net_addr sources[CONFIG_NET_IF_MCAST_IPV4_SOURCE_COUNT];

	/** Number of sources to be used by the filter */
	uint16_t sources_len;

	/** Filter mode (used in IGMPV3) */
	uint8_t record_type;
#endif

	/** Is this multicast IP address used or not */
	uint8_t is_used : 1;

	/** Did we join to this group */
	uint8_t is_joined : 1;

	uint8_t _unused : 6;
};

/**
 * @brief Network Interface IPv6 prefixes
 *
 * Stores the IPV6 prefixes assigned to this network interface.
 */
struct net_if_ipv6_prefix {
	/** Prefix lifetime */
	struct net_timeout lifetime;

	/** IPv6 prefix */
	struct in6_addr prefix;

	/** Backpointer to network interface where this prefix is used */
	struct net_if *iface;

	/** Prefix length */
	uint8_t len;

	/** Is the IP prefix valid forever */
	uint8_t is_infinite : 1;

	/** Is this prefix used or not */
	uint8_t is_used : 1;

	uint8_t _unused : 6;
};

/**
 * @brief Information about routers in the system.
 *
 * Stores the router information.
 */
struct net_if_router {
	/** Slist lifetime timer node */
	sys_snode_t node;

	/** IP address */
	struct net_addr address;

	/** Network interface the router is connected to */
	struct net_if *iface;

	/** Router life timer start */
	uint32_t life_start;

	/** Router lifetime */
	uint16_t lifetime;

	/** Is this router used or not */
	uint8_t is_used : 1;

	/** Is default router */
	uint8_t is_default : 1;

	/** Is the router valid forever */
	uint8_t is_infinite : 1;

	uint8_t _unused : 5;
};

/** Network interface flags. */
enum net_if_flag {
	/** Interface is admin up. */
	NET_IF_UP,

	/** Interface is pointopoint */
	NET_IF_POINTOPOINT,

	/** Interface is in promiscuous mode */
	NET_IF_PROMISC,

	/** Do not start the interface immediately after initialization.
	 * This requires that either the device driver or some other entity
	 * will need to manually take the interface up when needed.
	 * For example for Ethernet this will happen when the driver calls
	 * the net_eth_carrier_on() function.
	 */
	NET_IF_NO_AUTO_START,

	/** Power management specific: interface is being suspended */
	NET_IF_SUSPENDED,

	/** Flag defines if received multicasts of other interface are
	 * forwarded on this interface. This activates multicast
	 * routing / forwarding for this interface.
	 */
	NET_IF_FORWARD_MULTICASTS,

	/** Interface supports IPv4 */
	NET_IF_IPV4,

	/** Interface supports IPv6 */
	NET_IF_IPV6,

	/** Interface up and running (ready to receive and transmit). */
	NET_IF_RUNNING,

	/** Driver signals L1 is up. */
	NET_IF_LOWER_UP,

	/** Driver signals dormant. */
	NET_IF_DORMANT,

	/** IPv6 Neighbor Discovery disabled. */
	NET_IF_IPV6_NO_ND,

	/** IPv6 Multicast Listener Discovery disabled. */
	NET_IF_IPV6_NO_MLD,

	/** Mutex locking on TX data path disabled on the interface. */
	NET_IF_NO_TX_LOCK,

/** @cond INTERNAL_HIDDEN */
	/* Total number of flags - must be at the end of the enum */
	NET_IF_NUM_FLAGS
/** @endcond */
};

/** @brief Network interface operational status (RFC 2863). */
enum net_if_oper_state {
	NET_IF_OPER_UNKNOWN,        /**< Initial (unknown) value */
	NET_IF_OPER_NOTPRESENT,     /**< Hardware missing */
	NET_IF_OPER_DOWN,           /**< Interface is down */
	NET_IF_OPER_LOWERLAYERDOWN, /**< Lower layer interface is down */
	NET_IF_OPER_TESTING,        /**< Training mode */
	NET_IF_OPER_DORMANT,        /**< Waiting external action */
	NET_IF_OPER_UP,             /**< Interface is up */
} __packed;

#if defined(CONFIG_NET_OFFLOAD)
struct net_offload;
#endif /* CONFIG_NET_OFFLOAD */

/** @cond INTERNAL_HIDDEN */
#if defined(CONFIG_NET_IPV6)
#define NET_IF_MAX_IPV6_ADDR CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT
#define NET_IF_MAX_IPV6_MADDR CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT
#define NET_IF_MAX_IPV6_PREFIX CONFIG_NET_IF_IPV6_PREFIX_COUNT
#else
#define NET_IF_MAX_IPV6_ADDR 0
#define NET_IF_MAX_IPV6_MADDR 0
#define NET_IF_MAX_IPV6_PREFIX 0
#endif
/* @endcond */

/** IPv6 configuration */
struct net_if_ipv6 {
	/** Unicast IP addresses */
	struct net_if_addr unicast[NET_IF_MAX_IPV6_ADDR];

	/** Multicast IP addresses */
	struct net_if_mcast_addr mcast[NET_IF_MAX_IPV6_MADDR];

	/** Prefixes */
	struct net_if_ipv6_prefix prefix[NET_IF_MAX_IPV6_PREFIX];

	/** Default reachable time (RFC 4861, page 52) */
	uint32_t base_reachable_time;

	/** Reachable time (RFC 4861, page 20) */
	uint32_t reachable_time;

	/** Retransmit timer (RFC 4861, page 52) */
	uint32_t retrans_timer;

#if defined(CONFIG_NET_IPV6_IID_STABLE)
	/** IID (Interface Identifier) pointer used for link local address */
	struct net_if_addr *iid;

	/** Incremented when network interface goes down so that we can
	 * generate new stable addresses when interface comes back up.
	 */
	uint32_t network_counter;
#endif /* CONFIG_NET_IPV6_IID_STABLE */

#if defined(CONFIG_NET_IPV6_PE)
	/** Privacy extension DESYNC_FACTOR value from RFC 8981 ch 3.4.
	 * "DESYNC_FACTOR is a random value within the range 0 - MAX_DESYNC_FACTOR.
	 * It is computed every time a temporary address is created.
	 */
	uint32_t desync_factor;
#endif /* CONFIG_NET_IPV6_PE */

#if defined(CONFIG_NET_IPV6_ND) && defined(CONFIG_NET_NATIVE_IPV6)
	/** Router solicitation timer node */
	sys_snode_t rs_node;

	/* RS start time */
	uint32_t rs_start;

	/** RS count */
	uint8_t rs_count;
#endif

	/** IPv6 hop limit */
	uint8_t hop_limit;

	/** IPv6 multicast hop limit */
	uint8_t mcast_hop_limit;
};

#if defined(CONFIG_NET_DHCPV6) && defined(CONFIG_NET_NATIVE_IPV6)
/** DHCPv6 configuration */
struct net_if_dhcpv6 {
	/** Used for timer list. */
	sys_snode_t node;

	/** Generated Client ID. */
	struct net_dhcpv6_duid_storage clientid;

	/** Server ID of the selected server. */
	struct net_dhcpv6_duid_storage serverid;

	/** DHCPv6 client state. */
	enum net_dhcpv6_state state;

	/** DHCPv6 client configuration parameters. */
	struct net_dhcpv6_params params;

	/** Timeout for the next event, absolute time, milliseconds. */
	uint64_t timeout;

	/** Time of the current exchange start, absolute time, milliseconds */
	uint64_t exchange_start;

	/** Renewal time, absolute time, milliseconds. */
	uint64_t t1;

	/** Rebinding time, absolute time, milliseconds. */
	uint64_t t2;

	/** The time when the last lease expires (terminates rebinding,
	 *  DHCPv6 RFC8415, ch. 18.2.5). Absolute time, milliseconds.
	 */
	uint64_t expire;

	/** Generated IAID for IA_NA. */
	uint32_t addr_iaid;

	/** Generated IAID for IA_PD. */
	uint32_t prefix_iaid;

	/** Retransmit timeout for the current message, milliseconds. */
	uint32_t retransmit_timeout;

	/** Current best server preference received. */
	int16_t server_preference;

	/** Retransmission counter. */
	uint8_t retransmissions;

	/** Transaction ID for current exchange. */
	uint8_t tid[DHCPV6_TID_SIZE];

	/** Prefix length. */
	uint8_t prefix_len;

	/** Assigned IPv6 prefix. */
	struct in6_addr prefix;

	/** Assigned IPv6 address. */
	struct in6_addr addr;
};
#endif /* defined(CONFIG_NET_DHCPV6) && defined(CONFIG_NET_NATIVE_IPV6) */

/** @cond INTERNAL_HIDDEN */
#if defined(CONFIG_NET_IPV4)
#define NET_IF_MAX_IPV4_ADDR CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT
#define NET_IF_MAX_IPV4_MADDR CONFIG_NET_IF_MCAST_IPV4_ADDR_COUNT
#else
#define NET_IF_MAX_IPV4_ADDR 0
#define NET_IF_MAX_IPV4_MADDR 0
#endif
/** @endcond */

/**
 * @brief Network Interface unicast IPv4 address and netmask
 *
 * Stores the unicast IPv4 address and related netmask.
 */
struct net_if_addr_ipv4 {
	/** IPv4 address */
	struct net_if_addr ipv4;
	/** Netmask */
	struct in_addr netmask;
};

/** IPv4 configuration */
struct net_if_ipv4 {
	/** Unicast IP addresses */
	struct net_if_addr_ipv4 unicast[NET_IF_MAX_IPV4_ADDR];

	/** Multicast IP addresses */
	struct net_if_mcast_addr mcast[NET_IF_MAX_IPV4_MADDR];

	/** Gateway */
	struct in_addr gw;

	/** IPv4 time-to-live */
	uint8_t ttl;

	/** IPv4 time-to-live for multicast packets */
	uint8_t mcast_ttl;

#if defined(CONFIG_NET_IPV4_ACD)
	/** IPv4 conflict count.  */
	uint8_t conflict_cnt;
#endif
};

#if defined(CONFIG_NET_DHCPV4) && defined(CONFIG_NET_NATIVE_IPV4)
struct net_if_dhcpv4 {
	/** Used for timer lists */
	sys_snode_t node;

	/** Timer start */
	int64_t timer_start;

	/** Time for INIT, INIT-REBOOT, DISCOVER, REQUESTING, RENEWAL */
	uint32_t request_time;

	uint32_t xid;

	/** IP address Lease time */
	uint32_t lease_time;

	/** IP address Renewal time */
	uint32_t renewal_time;

	/** IP address Rebinding time */
	uint32_t rebinding_time;

	/** Server ID */
	struct in_addr server_id;

	/** Requested IP addr */
	struct in_addr requested_ip;

	/** Received netmask from the server */
	struct in_addr netmask;

	/**
	 *  DHCPv4 client state in the process of network
	 *  address allocation.
	 */
	enum net_dhcpv4_state state;

	/** Number of attempts made for REQUEST and RENEWAL messages */
	uint8_t attempts;

	/** The address of the server the request is sent to */
	struct in_addr request_server_addr;

	/** The source address of a received DHCP message */
	struct in_addr response_src_addr;

#ifdef CONFIG_NET_DHCPV4_OPTION_NTP_SERVER
	/** NTP server address */
	struct in_addr ntp_addr;
#endif
};
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV4_AUTO) && defined(CONFIG_NET_NATIVE_IPV4)
struct net_if_ipv4_autoconf {
	/** Backpointer to correct network interface */
	struct net_if *iface;

	/** Requested IP addr */
	struct in_addr requested_ip;

	/** IPV4 Autoconf state in the process of network address allocation.
	 */
	enum net_ipv4_autoconf_state state;
};
#endif /* CONFIG_NET_IPV4_AUTO */

/** @cond INTERNAL_HIDDEN */
/* We always need to have at least one IP config */
#define NET_IF_MAX_CONFIGS 1
/** @endcond */

/**
 * @brief Network interface IP address configuration.
 */
struct net_if_ip {
#if defined(CONFIG_NET_IPV6)
	struct net_if_ipv6 *ipv6;
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	struct net_if_ipv4 *ipv4;
#endif /* CONFIG_NET_IPV4 */
};

/**
 * @brief IP and other configuration related data for network interface.
 */
struct net_if_config {
#if defined(CONFIG_NET_IP)
	/** IP address configuration setting */
	struct net_if_ip ip;
#endif

#if defined(CONFIG_NET_DHCPV4) && defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if_dhcpv4 dhcpv4;
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_DHCPV6) && defined(CONFIG_NET_NATIVE_IPV6)
	struct net_if_dhcpv6 dhcpv6;
#endif /* CONFIG_NET_DHCPV6 */

#if defined(CONFIG_NET_IPV4_AUTO) && defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if_ipv4_autoconf ipv4auto;
#endif /* CONFIG_NET_IPV4_AUTO */

#if defined(CONFIG_NET_L2_VIRTUAL)
	/**
	 * This list keeps track of the virtual network interfaces
	 * that are attached to this network interface.
	 */
	sys_slist_t virtual_interfaces;
#endif /* CONFIG_NET_L2_VIRTUAL */

#if defined(CONFIG_NET_INTERFACE_NAME)
	/**
	 * Network interface can have a name and it is possible
	 * to search a network interface using this name.
	 */
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
#endif
};

/**
 * @brief Network traffic class.
 *
 * Traffic classes are used when sending or receiving data that is classified
 * with different priorities. So some traffic can be marked as high priority
 * and it will be sent or received first. Each network packet that is
 * transmitted or received goes through a fifo to a thread that will transmit
 * it.
 */
struct net_traffic_class {
	/** Fifo for handling this Tx or Rx packet */
	struct k_fifo fifo;

#if NET_TC_COUNT > 1 || defined(CONFIG_NET_TC_TX_SKIP_FOR_HIGH_PRIO) \
	|| defined(CONFIG_NET_TC_RX_SKIP_FOR_HIGH_PRIO)
	/** Semaphore for tracking the available slots in the fifo */
	struct k_sem fifo_slot;
#endif

	/** Traffic class handler thread */
	struct k_thread handler;

	/** Stack for this handler */
	k_thread_stack_t *stack;
};

/**
 * @typedef net_socket_create_t

 * @brief A function prototype to create an offloaded socket. The prototype is
 *        compatible with socket() function.
 */
typedef int (*net_socket_create_t)(int, int, int);

/**
 * @brief Network Interface Device structure
 *
 * Used to handle a network interface on top of a device driver instance.
 * There can be many net_if_dev instance against the same device.
 *
 * Such interface is mainly to be used by the link layer, but is also tight
 * to a network context: it then makes the relation with a network context
 * and the network device.
 *
 * Because of the strong relationship between a device driver and such
 * network interface, each net_if_dev should be instantiated by one of the
 * network device init macros found in net_if.h.
 */
struct net_if_dev {
	/** The actually device driver instance the net_if is related to */
	const struct device *dev;

	/** Interface's L2 layer */
	const struct net_l2 * const l2;

	/** Interface's private L2 data pointer */
	void *l2_data;

	/** For internal use */
	ATOMIC_DEFINE(flags, NET_IF_NUM_FLAGS);

	/** The hardware link address */
	struct net_linkaddr link_addr;

#if defined(CONFIG_NET_OFFLOAD)
	/** TCP/IP Offload functions.
	 * If non-NULL, then the TCP/IP stack is located
	 * in the communication chip that is accessed via this
	 * network interface.
	 */
	struct net_offload *offload;
#endif /* CONFIG_NET_OFFLOAD */

	/** The hardware MTU */
	uint16_t mtu;

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	/** A function pointer to create an offloaded socket.
	 *  If non-NULL, the interface is considered offloaded at socket level.
	 */
	net_socket_create_t socket_offload;
#endif /* CONFIG_NET_SOCKETS_OFFLOAD */

	/** RFC 2863 operational status */
	enum net_if_oper_state oper_state;

	/** Last time the operational state was changed.
	 * This is used to determine how long the interface has been in the
	 * current operational state.
	 *
	 * This value is set to 0 when the interface is created, and then
	 * updated whenever the operational state changes.
	 *
	 * The value is in milliseconds since boot.
	 */
	int64_t oper_state_change_time;
};

/**
 * @brief Network Interface structure
 *
 * Used to handle a network interface on top of a net_if_dev instance.
 * There can be many net_if instance against the same net_if_dev instance.
 *
 */
struct net_if {
	/** The net_if_dev instance the net_if is related to */
	struct net_if_dev *if_dev;

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	/** Network statistics related to this network interface */
	struct net_stats stats;

	/** Promethus collector for this network interface */
	IF_ENABLED(CONFIG_NET_STATISTICS_VIA_PROMETHEUS,
		   (struct prometheus_collector *collector);)
#endif /* CONFIG_NET_STATISTICS_PER_INTERFACE */

	/** Network interface instance configuration */
	struct net_if_config config;

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	/** Keep track of packets pending in traffic queues. This is
	 * needed to avoid putting network device driver to sleep if
	 * there are packets waiting to be sent.
	 */
	int tx_pending;
#endif

	/** Mutex protecting this network interface instance */
	struct k_mutex lock;

	/** Mutex used when sending data */
	struct k_mutex tx_lock;

	/** Network interface specific flags */
	/** Enable IPv6 privacy extension (RFC 8981), this is enabled
	 * by default if PE support is enabled in configuration.
	 */
	uint8_t pe_enabled : 1;

	/** If PE is enabled, then this tells whether public addresses
	 * are preferred over temporary ones for this interface.
	 */
	uint8_t pe_prefer_public : 1;

	/** Unused bit flags (ignore) */
	uint8_t _unused : 6;
};

/** @cond INTERNAL_HIDDEN */

static inline void net_if_lock(struct net_if *iface)
{
	NET_ASSERT(iface);

	(void)k_mutex_lock(&iface->lock, K_FOREVER);
}

static inline void net_if_unlock(struct net_if *iface)
{
	NET_ASSERT(iface);

	k_mutex_unlock(&iface->lock);
}

static inline bool net_if_flag_is_set(struct net_if *iface,
				      enum net_if_flag value);

static inline void net_if_tx_lock(struct net_if *iface)
{
	NET_ASSERT(iface);

	if (net_if_flag_is_set(iface, NET_IF_NO_TX_LOCK)) {
		return;
	}

	(void)k_mutex_lock(&iface->tx_lock, K_FOREVER);
}

static inline void net_if_tx_unlock(struct net_if *iface)
{
	NET_ASSERT(iface);

	if (net_if_flag_is_set(iface, NET_IF_NO_TX_LOCK)) {
		return;
	}

	k_mutex_unlock(&iface->tx_lock);
}

/** @endcond */

/**
 * @brief Set a value in network interface flags
 *
 * @param iface Pointer to network interface
 * @param value Flag value
 */
static inline void net_if_flag_set(struct net_if *iface,
				   enum net_if_flag value)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return;
	}

	atomic_set_bit(iface->if_dev->flags, value);
}

/**
 * @brief Test and set a value in network interface flags
 *
 * @param iface Pointer to network interface
 * @param value Flag value
 *
 * @return true if the bit was set, false if it wasn't.
 */
static inline bool net_if_flag_test_and_set(struct net_if *iface,
					    enum net_if_flag value)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return false;
	}

	return atomic_test_and_set_bit(iface->if_dev->flags, value);
}

/**
 * @brief Clear a value in network interface flags
 *
 * @param iface Pointer to network interface
 * @param value Flag value
 */
static inline void net_if_flag_clear(struct net_if *iface,
				     enum net_if_flag value)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return;
	}

	atomic_clear_bit(iface->if_dev->flags, value);
}

/**
 * @brief Test and clear a value in network interface flags
 *
 * @param iface Pointer to network interface
 * @param value Flag value
 *
 * @return true if the bit was set, false if it wasn't.
 */
static inline bool net_if_flag_test_and_clear(struct net_if *iface,
					      enum net_if_flag value)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return false;
	}

	return atomic_test_and_clear_bit(iface->if_dev->flags, value);
}

/**
 * @brief Check if a value in network interface flags is set
 *
 * @param iface Pointer to network interface
 * @param value Flag value
 *
 * @return True if the value is set, false otherwise
 */
static inline bool net_if_flag_is_set(struct net_if *iface,
				      enum net_if_flag value)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return false;
	}

	return atomic_test_bit(iface->if_dev->flags, value);
}

/**
 * @brief Set an operational state on an interface
 *
 * @param iface Pointer to network interface
 * @param oper_state Operational state to set
 *
 * @return The new operational state of an interface
 */
static inline enum net_if_oper_state net_if_oper_state_set(
	struct net_if *iface, enum net_if_oper_state oper_state)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NET_IF_OPER_UNKNOWN;
	}

	if (oper_state <= NET_IF_OPER_UP) {
		iface->if_dev->oper_state = oper_state;
	}

	net_if_lock(iface);

	iface->if_dev->oper_state_change_time = k_uptime_get();

	net_if_unlock(iface);

	return iface->if_dev->oper_state;
}

/**
 * @brief Get an operational state of an interface
 *
 * @param iface Pointer to network interface
 *
 * @return Operational state of an interface
 */
static inline enum net_if_oper_state net_if_oper_state(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NET_IF_OPER_UNKNOWN;
	}

	return iface->if_dev->oper_state;
}

/**
 * @brief Get an operational state change time of an interface
 *
 * @param iface Pointer to network interface
 * @param change_time Pointer to store the change time. Time the operational
 *        state of an interface was last changed, in milliseconds since boot.
 *        If the interface operational state has not been changed yet,
 *        then the value is 0.
 *
 * @return 0 if ok, -EINVAL if operational state change time could not be
 *         retrieved (for example if iface or change_time is NULL).
 */
static inline int net_if_oper_state_change_time(struct net_if *iface,
						int64_t *change_time)
{
	if (iface == NULL || iface->if_dev == NULL || change_time == NULL) {
		return -EINVAL;
	}

	net_if_lock(iface);

	*change_time = iface->if_dev->oper_state_change_time;

	net_if_unlock(iface);

	return 0;
}

/**
 * @brief Try sending a packet through a net iface
 *
 * @param iface Pointer to a network interface structure
 * @param pkt Pointer to a net packet to send
 * @param timeout timeout for attempting to send
 *
 * @return verdict about the packet
 */
enum net_verdict net_if_try_send_data(struct net_if *iface,
				      struct net_pkt *pkt, k_timeout_t timeout);

/**
 * @brief Send a packet through a net iface
 *
 * This is equivalent to net_if_try_queue_tx with an infinite timeout
 * @param iface Pointer to a network interface structure
 * @param pkt Pointer to a net packet to send
 *
 * @return verdict about the packet
 */
static inline enum net_verdict net_if_send_data(struct net_if *iface,
						struct net_pkt *pkt)
{
	k_timeout_t timeout = k_is_in_isr() ? K_NO_WAIT : K_FOREVER;

	return net_if_try_send_data(iface, pkt, timeout);
}

/**
 * @brief Get a pointer to the interface L2
 *
 * @param iface a valid pointer to a network interface structure
 *
 * @return a pointer to the iface L2
 */
static inline const struct net_l2 *net_if_l2(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return iface->if_dev->l2;
}

/**
 * @brief Input a packet through a net iface
 *
 * @param iface Pointer to a network interface structure
 * @param pkt Pointer to a net packet to input
 *
 * @return verdict about the packet
 */
enum net_verdict net_if_recv_data(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Get a pointer to the interface L2 private data
 *
 * @param iface a valid pointer to a network interface structure
 *
 * @return a pointer to the iface L2 data
 */
static inline void *net_if_l2_data(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return iface->if_dev->l2_data;
}

/**
 * @brief Get an network interface's device
 *
 * @param iface Pointer to a network interface structure
 *
 * @return a pointer to the device driver instance
 */
static inline const struct device *net_if_get_device(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return iface->if_dev->dev;
}

/**
 * @brief Try enqueuing a packet to the net interface TX queue
 *
 * @param iface Pointer to a network interface structure
 * @param pkt Pointer to a net packet to queue
 * @param timeout Timeout for the enqueuing attempt
 */
void net_if_try_queue_tx(struct net_if *iface, struct net_pkt *pkt, k_timeout_t timeout);

/**
 * @brief Queue a packet to the net interface TX queue
 *
 * This is equivalent to net_if_try_queue_tx with an infinite timeout
 * @param iface Pointer to a network interface structure
 * @param pkt Pointer to a net packet to queue
 */
static inline void net_if_queue_tx(struct net_if *iface, struct net_pkt *pkt)
{
	k_timeout_t timeout = k_is_in_isr() ? K_NO_WAIT : K_FOREVER;

	net_if_try_queue_tx(iface, pkt, timeout);
}

/**
 * @brief Return the IP offload status
 *
 * @param iface Network interface
 *
 * @return True if IP offloading is active, false otherwise.
 */
static inline bool net_if_is_ip_offloaded(struct net_if *iface)
{
#if defined(CONFIG_NET_OFFLOAD)
	return (iface != NULL && iface->if_dev != NULL &&
		iface->if_dev->offload != NULL);
#else
	ARG_UNUSED(iface);

	return false;
#endif
}

/**
 * @brief Return offload status of a given network interface.
 *
 * @param iface Network interface
 *
 * @return True if IP or socket offloading is active, false otherwise.
 */
bool net_if_is_offloaded(struct net_if *iface);

/**
 * @brief Return the IP offload plugin
 *
 * @param iface Network interface
 *
 * @return NULL if there is no offload plugin defined, valid pointer otherwise
 */
static inline struct net_offload *net_if_offload(struct net_if *iface)
{
#if defined(CONFIG_NET_OFFLOAD)
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return iface->if_dev->offload;
#else
	ARG_UNUSED(iface);

	return NULL;
#endif
}

/**
 * @brief Return the socket offload status
 *
 * @param iface Network interface
 *
 * @return True if socket offloading is active, false otherwise.
 */
static inline bool net_if_is_socket_offloaded(struct net_if *iface)
{
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	if (iface == NULL || iface->if_dev == NULL) {
		return false;
	}

	return (iface->if_dev->socket_offload != NULL);
#else
	ARG_UNUSED(iface);

	return false;
#endif
}

/**
 * @brief Set the function to create an offloaded socket
 *
 * @param iface Network interface
 * @param socket_offload A function to create an offloaded socket
 */
static inline void net_if_socket_offload_set(
		struct net_if *iface, net_socket_create_t socket_offload)
{
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	if (iface == NULL || iface->if_dev == NULL) {
		return;
	}

	iface->if_dev->socket_offload = socket_offload;
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(socket_offload);
#endif
}

/**
 * @brief Return the function to create an offloaded socket
 *
 * @param iface Network interface
 *
 * @return NULL if the interface is not socket offloaded, valid pointer otherwise
 */
static inline net_socket_create_t net_if_socket_offload(struct net_if *iface)
{
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return iface->if_dev->socket_offload;
#else
	ARG_UNUSED(iface);

	return NULL;
#endif
}

/**
 * @brief Get an network interface's link address
 *
 * @param iface Pointer to a network interface structure
 *
 * @return a pointer to the network link address
 */
static inline struct net_linkaddr *net_if_get_link_addr(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return NULL;
	}

	return &iface->if_dev->link_addr;
}

/**
 * @brief Return network configuration for this network interface
 *
 * @param iface Pointer to a network interface structure
 *
 * @return Pointer to configuration
 */
static inline struct net_if_config *net_if_get_config(struct net_if *iface)
{
	if (iface == NULL) {
		return NULL;
	}

	return &iface->config;
}

/**
 * @brief Start duplicate address detection procedure.
 *
 * @param iface Pointer to a network interface structure
 */
#if defined(CONFIG_NET_IPV6_DAD) && defined(CONFIG_NET_NATIVE_IPV6)
void net_if_start_dad(struct net_if *iface);
#else
static inline void net_if_start_dad(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Start neighbor discovery and send router solicitation message.
 *
 * @param iface Pointer to a network interface structure
 */
void net_if_start_rs(struct net_if *iface);


/**
 * @brief Stop neighbor discovery.
 *
 * @param iface Pointer to a network interface structure
 */
#if defined(CONFIG_NET_IPV6_ND) && defined(CONFIG_NET_NATIVE_IPV6)
void net_if_stop_rs(struct net_if *iface);
#else
static inline void net_if_stop_rs(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif /* CONFIG_NET_IPV6_ND */

/**
 * @brief Provide a reachability hint for IPv6 Neighbor Discovery.
 *
 * This function is intended for upper-layer protocols to inform the IPv6
 * Neighbor Discovery process about an active link to a specific neighbor.
 * By signaling a recent "forward progress" event, such as the reception of
 * an ACK, this function can help reduce unnecessary ND traffic as per the
 * guidelines in RFC 4861 (section 7.3).
 *
 * @param iface A pointer to the network interface.
 * @param ipv6_addr Pointer to the IPv6 address of the neighbor node.
 */
#if defined(CONFIG_NET_IPV6_ND) && defined(CONFIG_NET_NATIVE_IPV6)
void net_if_nbr_reachability_hint(struct net_if *iface, const struct in6_addr *ipv6_addr);
#else
static inline void net_if_nbr_reachability_hint(struct net_if *iface,
						const struct in6_addr *ipv6_addr)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(ipv6_addr);
}
#endif

/** @cond INTERNAL_HIDDEN */

static inline int net_if_set_link_addr_unlocked(struct net_if *iface,
						uint8_t *addr, uint8_t len,
						enum net_link_type type)
{
	int ret;

	if (net_if_flag_is_set(iface, NET_IF_RUNNING)) {
		return -EPERM;
	}

	if (len > sizeof(net_if_get_link_addr(iface)->addr)) {
		return -EINVAL;
	}

	ret = net_linkaddr_create(net_if_get_link_addr(iface), addr, len, type);
	if (ret < 0) {
		return ret;
	}

	net_hostname_set_postfix(addr, len);

	return 0;
}

int net_if_set_link_addr_locked(struct net_if *iface,
				uint8_t *addr, uint8_t len,
				enum net_link_type type);

#if CONFIG_NET_IF_LOG_LEVEL >= LOG_LEVEL_DBG
extern int net_if_addr_unref_debug(struct net_if *iface,
				   sa_family_t family,
				   const void *addr,
				   struct net_if_addr **ifaddr,
				   const char *caller, int line);
#define net_if_addr_unref(iface, family, addr, ifaddr)			\
	net_if_addr_unref_debug(iface, family, addr, ifaddr, __func__, __LINE__)

extern struct net_if_addr *net_if_addr_ref_debug(struct net_if *iface,
						 sa_family_t family,
						 const void *addr,
						 const char *caller,
						 int line);
#define net_if_addr_ref(iface, family, addr) \
	net_if_addr_ref_debug(iface, family, addr, __func__, __LINE__)
#else
extern int net_if_addr_unref(struct net_if *iface,
			     sa_family_t family,
			     const void *addr,
			     struct net_if_addr **ifaddr);
extern struct net_if_addr *net_if_addr_ref(struct net_if *iface,
					   sa_family_t family,
					   const void *addr);
#endif /* CONFIG_NET_IF_LOG_LEVEL */

/** @endcond */

/**
 * @brief Set a network interface's link address
 *
 * @param iface Pointer to a network interface structure
 * @param addr A pointer to a uint8_t buffer representing the address.
 *             The buffer must remain valid throughout interface lifetime.
 * @param len length of the address buffer
 * @param type network bearer type of this link address
 *
 * @return 0 on success
 */
static inline int net_if_set_link_addr(struct net_if *iface,
				       uint8_t *addr, uint8_t len,
				       enum net_link_type type)
{
#if defined(CONFIG_NET_RAW_MODE)
	return net_if_set_link_addr_unlocked(iface, addr, len, type);
#else
	return net_if_set_link_addr_locked(iface, addr, len, type);
#endif
}

/**
 * @brief Get an network interface's MTU
 *
 * @param iface Pointer to a network interface structure
 *
 * @return the MTU
 */
static inline uint16_t net_if_get_mtu(struct net_if *iface)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return 0U;
	}

	return iface->if_dev->mtu;
}

/**
 * @brief Set an network interface's MTU
 *
 * @param iface Pointer to a network interface structure
 * @param mtu New MTU, note that we store only 16 bit mtu value.
 */
static inline void net_if_set_mtu(struct net_if *iface,
				  uint16_t mtu)
{
	if (iface == NULL || iface->if_dev == NULL) {
		return;
	}

	iface->if_dev->mtu = mtu;
}

/**
 * @brief Set the infinite status of the network interface address
 *
 * @param ifaddr IP address for network interface
 * @param is_infinite Infinite status
 */
static inline void net_if_addr_set_lf(struct net_if_addr *ifaddr,
				      bool is_infinite)
{
	if (ifaddr == NULL) {
		return;
	}

	ifaddr->is_infinite = is_infinite;
}

/**
 * @brief Get an interface according to link layer address.
 *
 * @param ll_addr Link layer address.
 *
 * @return Network interface or NULL if not found.
 */
struct net_if *net_if_get_by_link_addr(struct net_linkaddr *ll_addr);

/**
 * @brief Find an interface from it's related device
 *
 * @param dev A valid struct device pointer to relate with an interface
 *
 * @return a valid struct net_if pointer on success, NULL otherwise
 */
struct net_if *net_if_lookup_by_dev(const struct device *dev);

/**
 * @brief Get network interface IP config
 *
 * @param iface Interface to use.
 *
 * @return NULL if not found or pointer to correct config settings.
 */
static inline struct net_if_config *net_if_config_get(struct net_if *iface)
{
	if (iface == NULL) {
		return NULL;
	}

	return &iface->config;
}

/**
 * @brief Remove a router from the system
 *
 * @param router Pointer to existing router
 */
void net_if_router_rm(struct net_if_router *router);

/**
 * @brief Set the default network interface.
 *
 * @param iface New default interface, or NULL to revert to the one set by Kconfig.
 */
void net_if_set_default(struct net_if *iface);

/**
 * @brief Get the default network interface.
 *
 * @return Default interface or NULL if no interfaces are configured.
 */
struct net_if *net_if_get_default(void);

/**
 * @brief Get the first network interface according to its type.
 *
 * @param l2 Layer 2 type of the network interface.
 *
 * @return First network interface of a given type or NULL if no such
 * interfaces was found.
 */
struct net_if *net_if_get_first_by_type(const struct net_l2 *l2);

/**
 * @brief Get the first network interface which is up.
 *
 * @return First network interface which is up or NULL if all
 * interfaces are down.
 */
struct net_if *net_if_get_first_up(void);

#if defined(CONFIG_NET_L2_IEEE802154)
/**
 * @brief Get the first IEEE 802.15.4 network interface.
 *
 * @return First IEEE 802.15.4 network interface or NULL if no such
 * interfaces was found.
 */
static inline struct net_if *net_if_get_ieee802154(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(IEEE802154));
}
#endif /* CONFIG_NET_L2_IEEE802154 */

/**
 * @brief Allocate network interface IPv6 config.
 *
 * @details This function will allocate new IPv6 config.
 *
 * @param iface Interface to use.
 * @param ipv6 Pointer to allocated IPv6 struct is returned to caller.
 *
 * @return 0 if ok, <0 if error
 */
int net_if_config_ipv6_get(struct net_if *iface,
			   struct net_if_ipv6 **ipv6);

/**
 * @brief Release network interface IPv6 config.
 *
 * @param iface Interface to use.
 *
 * @return 0 if ok, <0 if error
 */
int net_if_config_ipv6_put(struct net_if *iface);


/** @cond INTERNAL_HIDDEN */
struct net_if_addr *net_if_ipv6_addr_lookup_raw(const uint8_t *addr,
						struct net_if **ret);
/** @endcond */

/**
 * @brief Check if this IPv6 address belongs to one of the interfaces.
 *
 * @param addr IPv6 address
 * @param iface Pointer to interface is returned
 *
 * @return Pointer to interface address, NULL if not found.
 */
struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **iface);

/** @cond INTERNAL_HIDDEN */
struct net_if_addr *net_if_ipv6_addr_lookup_by_iface_raw(struct net_if *iface,
							 const uint8_t *addr);
/** @endcond */

/**
 * @brief Check if this IPv6 address belongs to this specific interfaces.
 *
 * @param iface Network interface
 * @param addr IPv6 address
 *
 * @return Pointer to interface address, NULL if not found.
 */
struct net_if_addr *net_if_ipv6_addr_lookup_by_iface(struct net_if *iface,
						     const struct in6_addr *addr);

/**
 * @brief Check if this IPv6 address belongs to one of the interface indices.
 *
 * @param addr IPv6 address
 *
 * @return >0 if address was found in given network interface index,
 * all other values mean address was not found
 */
__syscall int net_if_ipv6_addr_lookup_by_index(const struct in6_addr *addr);

/**
 * @brief Add a IPv6 address to an interface
 *
 * @param iface Network interface
 * @param addr IPv6 address
 * @param addr_type IPv6 address type
 * @param vlifetime Validity time for this address
 *
 * @return Pointer to interface address, NULL if cannot be added
 */
struct net_if_addr *net_if_ipv6_addr_add(struct net_if *iface,
					 const struct in6_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime);

/**
 * @brief Add a IPv6 address to an interface by index
 *
 * @param index Network interface index
 * @param addr IPv6 address
 * @param addr_type IPv6 address type
 * @param vlifetime Validity time for this address
 *
 * @return True if ok, false if address could not be added
 */
__syscall bool net_if_ipv6_addr_add_by_index(int index,
					     const struct in6_addr *addr,
					     enum net_addr_type addr_type,
					     uint32_t vlifetime);

/**
 * @brief Update validity lifetime time of an IPv6 address.
 *
 * @param ifaddr Network IPv6 address
 * @param vlifetime Validity time for this address
 */
void net_if_ipv6_addr_update_lifetime(struct net_if_addr *ifaddr,
				      uint32_t vlifetime);

/**
 * @brief Remove an IPv6 address from an interface
 *
 * @param iface Network interface
 * @param addr IPv6 address
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv6_addr_rm(struct net_if *iface, const struct in6_addr *addr);

/**
 * @brief Remove an IPv6 address from an interface by index
 *
 * @param index Network interface index
 * @param addr IPv6 address
 *
 * @return True if successfully removed, false otherwise
 */
__syscall bool net_if_ipv6_addr_rm_by_index(int index,
					    const struct in6_addr *addr);

/**
 * @typedef net_if_ip_addr_cb_t
 * @brief Callback used while iterating over network interface IP addresses
 *
 * @param iface Pointer to the network interface the address belongs to
 * @param addr Pointer to current IP address
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_if_ip_addr_cb_t)(struct net_if *iface,
				    struct net_if_addr *addr,
				    void *user_data);

/**
 * @brief Go through all IPv6 addresses on a network interface and call callback
 * for each used address.
 *
 * @param iface Pointer to the network interface
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_if_ipv6_addr_foreach(struct net_if *iface, net_if_ip_addr_cb_t cb,
			      void *user_data);

/**
 * @brief Add a IPv6 multicast address to an interface
 *
 * @param iface Network interface
 * @param addr IPv6 multicast address
 *
 * @return Pointer to interface multicast address, NULL if cannot be added
 */
struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						const struct in6_addr *addr);

/**
 * @brief Remove an IPv6 multicast address from an interface
 *
 * @param iface Network interface
 * @param addr IPv6 multicast address
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv6_maddr_rm(struct net_if *iface, const struct in6_addr *addr);

/**
 * @typedef net_if_ip_maddr_cb_t
 * @brief Callback used while iterating over network interface multicast IP addresses
 *
 * @param iface Pointer to the network interface the address belongs to
 * @param maddr Pointer to current multicast IP address
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_if_ip_maddr_cb_t)(struct net_if *iface,
				     struct net_if_mcast_addr *maddr,
				     void *user_data);

/**
 * @brief Go through all IPv6 multicast addresses on a network interface and call
 * callback for each used address.
 *
 * @param iface Pointer to the network interface
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_if_ipv6_maddr_foreach(struct net_if *iface, net_if_ip_maddr_cb_t cb,
			       void *user_data);


/** @cond INTERNAL_HIDDEN */
struct net_if_mcast_addr *net_if_ipv6_maddr_lookup_raw(const uint8_t *maddr,
						       struct net_if **ret);
/** @endcond */

/**
 * @brief Check if this IPv6 multicast address belongs to a specific interface
 * or one of the interfaces.
 *
 * @param addr IPv6 address
 * @param iface If *iface is null, then pointer to interface is returned,
 * otherwise the *iface value needs to be matched.
 *
 * @return Pointer to interface multicast address, NULL if not found.
 */
struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(const struct in6_addr *addr,
						   struct net_if **iface);

/**
 * @typedef net_if_mcast_callback_t

 * @brief Define a callback that is called whenever a IPv6 or IPv4 multicast
 *        address group is joined or left.
 * @param iface A pointer to a struct net_if to which the multicast address is
 *        attached.
 * @param addr IP multicast address.
 * @param is_joined True if the multicast group is joined, false if group is left.
 */
typedef void (*net_if_mcast_callback_t)(struct net_if *iface,
					const struct net_addr *addr,
					bool is_joined);

/**
 * @brief Multicast monitor handler struct.
 *
 * Stores the multicast callback information. Caller must make sure that
 * the variable pointed by this is valid during the lifetime of
 * registration. Typically this means that the variable cannot be
 * allocated from stack.
 */
struct net_if_mcast_monitor {
	/** Node information for the slist. */
	sys_snode_t node;

	/** Network interface */
	struct net_if *iface;

	/** Multicast callback */
	net_if_mcast_callback_t cb;
};

/**
 * @brief Register a multicast monitor
 *
 * @param mon Monitor handle. This is a pointer to a monitor storage structure
 * which should be allocated by caller, but does not need to be initialized.
 * @param iface Network interface or NULL for all interfaces
 * @param cb Monitor callback
 */
void net_if_mcast_mon_register(struct net_if_mcast_monitor *mon,
			       struct net_if *iface,
			       net_if_mcast_callback_t cb);

/**
 * @brief Unregister a multicast monitor
 *
 * @param mon Monitor handle
 */
void net_if_mcast_mon_unregister(struct net_if_mcast_monitor *mon);

/**
 * @brief Call registered multicast monitors
 *
 * @param iface Network interface
 * @param addr Multicast address
 * @param is_joined Is this multicast address group joined (true) or not (false)
 */
void net_if_mcast_monitor(struct net_if *iface, const struct net_addr *addr,
			  bool is_joined);

/**
 * @brief Mark a given multicast address to be joined.
 *
 * @param iface Network interface the address belongs to
 * @param addr IPv6 multicast address
 */
void net_if_ipv6_maddr_join(struct net_if *iface,
			    struct net_if_mcast_addr *addr);

/**
 * @brief Check if given multicast address is joined or not.
 *
 * @param addr IPv6 multicast address
 *
 * @return True if address is joined, False otherwise.
 */
static inline bool net_if_ipv6_maddr_is_joined(struct net_if_mcast_addr *addr)
{
	if (addr == NULL) {
		return false;
	}

	return addr->is_joined;
}

/**
 * @brief Mark a given multicast address to be left.
 *
 * @param iface Network interface the address belongs to
 * @param addr IPv6 multicast address
 */
void net_if_ipv6_maddr_leave(struct net_if *iface,
			     struct net_if_mcast_addr *addr);

/**
 * @brief Return prefix that corresponds to this IPv6 address.
 *
 * @param iface Network interface
 * @param addr IPv6 address
 *
 * @return Pointer to prefix, NULL if not found.
 */
struct net_if_ipv6_prefix *net_if_ipv6_prefix_get(struct net_if *iface,
						  const struct in6_addr *addr);

/**
 * @brief Check if this IPv6 prefix belongs to this interface
 *
 * @param iface Network interface
 * @param addr IPv6 address
 * @param len Prefix length
 *
 * @return Pointer to prefix, NULL if not found.
 */
struct net_if_ipv6_prefix *net_if_ipv6_prefix_lookup(struct net_if *iface,
						     const struct in6_addr *addr,
						     uint8_t len);

/**
 * @brief Add a IPv6 prefix to an network interface.
 *
 * @param iface Network interface
 * @param prefix IPv6 address
 * @param len Prefix length
 * @param lifetime Prefix lifetime in seconds
 *
 * @return Pointer to prefix, NULL if the prefix was not added.
 */
struct net_if_ipv6_prefix *net_if_ipv6_prefix_add(struct net_if *iface,
						  const struct in6_addr *prefix,
						  uint8_t len,
						  uint32_t lifetime);

/**
 * @brief Remove an IPv6 prefix from an interface
 *
 * @param iface Network interface
 * @param addr IPv6 prefix address
 * @param len Prefix length
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv6_prefix_rm(struct net_if *iface, const struct in6_addr *addr,
			   uint8_t len);

/**
 * @brief Set the infinite status of the prefix
 *
 * @param prefix IPv6 address
 * @param is_infinite Infinite status
 */
static inline void net_if_ipv6_prefix_set_lf(struct net_if_ipv6_prefix *prefix,
					     bool is_infinite)
{
	prefix->is_infinite = is_infinite;
}

/**
 * @brief Set the prefix lifetime timer.
 *
 * @param prefix IPv6 address
 * @param lifetime Prefix lifetime in seconds
 */
void net_if_ipv6_prefix_set_timer(struct net_if_ipv6_prefix *prefix,
				  uint32_t lifetime);

/**
 * @brief Unset the prefix lifetime timer.
 *
 * @param prefix IPv6 address
 */
void net_if_ipv6_prefix_unset_timer(struct net_if_ipv6_prefix *prefix);

/**
 * @brief Check if this IPv6 address is part of the subnet of our
 * network interface.
 *
 * @param iface Network interface. This is returned to the caller.
 * The iface can be NULL in which case we check all the interfaces.
 * @param addr IPv6 address
 *
 * @return True if address is part of our subnet, false otherwise
 */
bool net_if_ipv6_addr_onlink(struct net_if **iface, const struct in6_addr *addr);

/**
 * @brief Get the IPv6 address of the given router
 * @param router a network router
 *
 * @return pointer to the IPv6 address, or NULL if none
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
static inline struct in6_addr *net_if_router_ipv6(struct net_if_router *router)
{
	if (router == NULL) {
		return NULL;
	}

	return &router->address.in6_addr;
}
#else
static inline struct in6_addr *net_if_router_ipv6(struct net_if_router *router)
{
	static struct in6_addr addr;

	ARG_UNUSED(router);

	return &addr;
}
#endif

/**
 * @brief Check if IPv6 address is one of the routers configured
 * in the system.
 *
 * @param iface Network interface
 * @param addr IPv6 address
 *
 * @return Pointer to router information, NULL if cannot be found
 */
struct net_if_router *net_if_ipv6_router_lookup(struct net_if *iface,
						const struct in6_addr *addr);

/**
 * @brief Find default router for this IPv6 address.
 *
 * @param iface Network interface. This can be NULL in which case we
 * go through all the network interfaces to find a suitable router.
 * @param addr IPv6 address
 *
 * @return Pointer to router information, NULL if cannot be found
 */
struct net_if_router *net_if_ipv6_router_find_default(struct net_if *iface,
						      const struct in6_addr *addr);

/**
 * @brief Update validity lifetime time of a router.
 *
 * @param router Network IPv6 address
 * @param lifetime Lifetime of this router.
 */
void net_if_ipv6_router_update_lifetime(struct net_if_router *router,
					uint16_t lifetime);

/**
 * @brief Add IPv6 router to the system.
 *
 * @param iface Network interface
 * @param addr IPv6 address
 * @param router_lifetime Lifetime of the router
 *
 * @return Pointer to router information, NULL if could not be added
 */
struct net_if_router *net_if_ipv6_router_add(struct net_if *iface,
					     const struct in6_addr *addr,
					     uint16_t router_lifetime);

/**
 * @brief Remove IPv6 router from the system.
 *
 * @param router Router information.
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv6_router_rm(struct net_if_router *router);

/**
 * @brief Get IPv6 hop limit specified for a given interface. This is the
 * default value but can be overridden by the user.
 *
 * @param iface Network interface
 *
 * @return Hop limit
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
uint8_t net_if_ipv6_get_hop_limit(struct net_if *iface);
#else
static inline uint8_t net_if_ipv6_get_hop_limit(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return 0;
}
#endif /* CONFIG_NET_NATIVE_IPV6 */

/**
 * @brief Set the default IPv6 hop limit of a given interface.
 *
 * @param iface Network interface
 * @param hop_limit New hop limit
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
void net_if_ipv6_set_hop_limit(struct net_if *iface, uint8_t hop_limit);
#else
static inline void net_if_ipv6_set_hop_limit(struct net_if *iface,
					     uint8_t hop_limit)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(hop_limit);
}
#endif /* CONFIG_NET_NATIVE_IPV6 */

/** @cond INTERNAL_HIDDEN */

/* The old hop limit setter function is deprecated because the naming
 * of it was incorrect. The API name was missing "_if_" so this function
 * should not be used.
 */
__deprecated
static inline void net_ipv6_set_hop_limit(struct net_if *iface,
					  uint8_t hop_limit)
{
	net_if_ipv6_set_hop_limit(iface, hop_limit);
}

/** @endcond */

/**
 * @brief Get IPv6 multicast hop limit specified for a given interface. This is the
 * default value but can be overridden by the user.
 *
 * @param iface Network interface
 *
 * @return Hop limit
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
uint8_t net_if_ipv6_get_mcast_hop_limit(struct net_if *iface);
#else
static inline uint8_t net_if_ipv6_get_mcast_hop_limit(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return 0;
}
#endif /* CONFIG_NET_NATIVE_IPV6 */

/**
 * @brief Set the default IPv6 multicast hop limit of a given interface.
 *
 * @param iface Network interface
 * @param hop_limit New hop limit
 */
#if defined(CONFIG_NET_NATIVE_IPV6)
void net_if_ipv6_set_mcast_hop_limit(struct net_if *iface, uint8_t hop_limit);
#else
static inline void net_if_ipv6_set_mcast_hop_limit(struct net_if *iface,
						   uint8_t hop_limit)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(hop_limit);
}
#endif /* CONFIG_NET_NATIVE_IPV6 */

/**
 * @brief Set IPv6 reachable time for a given interface
 *
 * @param iface Network interface
 * @param reachable_time New reachable time
 */
static inline void net_if_ipv6_set_base_reachable_time(struct net_if *iface,
						       uint32_t reachable_time)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	if (iface == NULL) {
		return;
	}

	if (!iface->config.ip.ipv6) {
		return;
	}

	iface->config.ip.ipv6->base_reachable_time = reachable_time;
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(reachable_time);

#endif
}

/**
 * @brief Get IPv6 reachable timeout specified for a given interface
 *
 * @param iface Network interface
 *
 * @return Reachable timeout
 */
static inline uint32_t net_if_ipv6_get_reachable_time(struct net_if *iface)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	if (iface == NULL) {
		return 0;
	}

	if (!iface->config.ip.ipv6) {
		return 0;
	}

	return iface->config.ip.ipv6->reachable_time;
#else
	ARG_UNUSED(iface);
	return 0;
#endif
}

/**
 * @brief Calculate next reachable time value for IPv6 reachable time
 *
 * @param ipv6 IPv6 address configuration
 *
 * @return Reachable time
 */
uint32_t net_if_ipv6_calc_reachable_time(struct net_if_ipv6 *ipv6);

/**
 * @brief Set IPv6 reachable time for a given interface. This requires
 * that base reachable time is set for the interface.
 *
 * @param ipv6 IPv6 address configuration
 */
static inline void net_if_ipv6_set_reachable_time(struct net_if_ipv6 *ipv6)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	if (ipv6 == NULL) {
		return;
	}

	ipv6->reachable_time = net_if_ipv6_calc_reachable_time(ipv6);
#else
	ARG_UNUSED(ipv6);
#endif
}

/**
 * @brief Set IPv6 retransmit timer for a given interface
 *
 * @param iface Network interface
 * @param retrans_timer New retransmit timer
 */
static inline void net_if_ipv6_set_retrans_timer(struct net_if *iface,
						 uint32_t retrans_timer)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	if (iface == NULL) {
		return;
	}

	if (!iface->config.ip.ipv6) {
		return;
	}

	iface->config.ip.ipv6->retrans_timer = retrans_timer;
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(retrans_timer);
#endif
}

/**
 * @brief Get IPv6 retransmit timer specified for a given interface
 *
 * @param iface Network interface
 *
 * @return Retransmit timer
 */
static inline uint32_t net_if_ipv6_get_retrans_timer(struct net_if *iface)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	if (iface == NULL) {
		return 0;
	}

	if (!iface->config.ip.ipv6) {
		return 0;
	}

	return iface->config.ip.ipv6->retrans_timer;
#else
	ARG_UNUSED(iface);
	return 0;
#endif
}

/**
 * @brief Get a IPv6 source address that should be used when sending
 * network data to destination.
 *
 * @param iface Interface that was used when packet was received.
 * If the interface is not known, then NULL can be given.
 * @param dst IPv6 destination address
 *
 * @return Pointer to IPv6 address to use, NULL if no IPv6 address
 * could be found.
 */
#if defined(CONFIG_NET_IPV6)
const struct in6_addr *net_if_ipv6_select_src_addr(struct net_if *iface,
						   const struct in6_addr *dst);
#else
static inline const struct in6_addr *net_if_ipv6_select_src_addr(
	struct net_if *iface, const struct in6_addr *dst)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);

	return NULL;
}
#endif

/**
 * @brief Get a IPv6 source address that should be used when sending
 * network data to destination. Use a hint set to the socket to select
 * the proper address.
 *
 * @param iface Interface that was used when packet was received.
 * If the interface is not known, then NULL can be given.
 * @param dst IPv6 destination address
 * @param flags Hint from the related socket. See RFC 5014 for value details.
 *
 * @return Pointer to IPv6 address to use, NULL if no IPv6 address
 * could be found.
 */
#if defined(CONFIG_NET_IPV6)
const struct in6_addr *net_if_ipv6_select_src_addr_hint(struct net_if *iface,
							const struct in6_addr *dst,
							int flags);
#else
static inline const struct in6_addr *net_if_ipv6_select_src_addr_hint(
	struct net_if *iface, const struct in6_addr *dst, int flags)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);
	ARG_UNUSED(flags);

	return NULL;
}
#endif

/**
 * @brief Get a network interface that should be used when sending
 * IPv6 network data to destination.
 *
 * @param dst IPv6 destination address
 *
 * @return Pointer to network interface to use, NULL if no suitable interface
 * could be found.
 */
#if defined(CONFIG_NET_IPV6)
struct net_if *net_if_ipv6_select_src_iface(const struct in6_addr *dst);
#else
static inline struct net_if *net_if_ipv6_select_src_iface(
	const struct in6_addr *dst)
{
	ARG_UNUSED(dst);

	return NULL;
}
#endif

/**
 * @brief Get a network interface that should be used when sending
 * IPv6 network data to destination. Also return the source IPv6 address from
 * that network interface.
 *
 * @param dst IPv6 destination address
 * @param src_addr IPv6 source address. This can be set to NULL if the source
 *                 address is not needed.
 *
 * @return Pointer to network interface to use, NULL if no suitable interface
 * could be found.
 */
#if defined(CONFIG_NET_IPV6)
struct net_if *net_if_ipv6_select_src_iface_addr(const struct in6_addr *dst,
						 const struct in6_addr **src_addr);
#else
static inline struct net_if *net_if_ipv6_select_src_iface_addr(
	const struct in6_addr *dst, const struct in6_addr **src_addr)
{
	ARG_UNUSED(dst);
	ARG_UNUSED(src_addr);

	return NULL;
}
#endif /* CONFIG_NET_IPV6 */

/**
 * @brief Get a IPv6 link local address in a given state.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr_state IPv6 address state (preferred, tentative, deprecated)
 *
 * @return Pointer to link local IPv6 address, NULL if no proper IPv6 address
 * could be found.
 */
struct in6_addr *net_if_ipv6_get_ll(struct net_if *iface,
				    enum net_addr_state addr_state);

/**
 * @brief Return link local IPv6 address from the first interface that has
 * a link local address matching give state.
 *
 * @param state IPv6 address state (ANY, TENTATIVE, PREFERRED, DEPRECATED)
 * @param iface Pointer to interface is returned
 *
 * @return Pointer to IPv6 address, NULL if not found.
 */
struct in6_addr *net_if_ipv6_get_ll_addr(enum net_addr_state state,
					 struct net_if **iface);

/**
 * @brief Stop IPv6 Duplicate Address Detection (DAD) procedure if
 * we find out that our IPv6 address is already in use.
 *
 * @param iface Interface where the DAD was running.
 * @param addr IPv6 address that failed DAD
 */
void net_if_ipv6_dad_failed(struct net_if *iface, const struct in6_addr *addr);

/**
 * @brief Return global IPv6 address from the first interface that has
 * a global IPv6 address matching the given state.
 *
 * @param state IPv6 address state (ANY, TENTATIVE, PREFERRED, DEPRECATED)
 * @param iface Caller can give an interface to check. If iface is set to NULL,
 * then all the interfaces are checked. Pointer to interface where the IPv6
 * address is defined is returned to the caller.
 *
 * @return Pointer to IPv6 address, NULL if not found.
 */
struct in6_addr *net_if_ipv6_get_global_addr(enum net_addr_state state,
					     struct net_if **iface);

/**
 * @brief Allocate network interface IPv4 config.
 *
 * @details This function will allocate new IPv4 config.
 *
 * @param iface Interface to use.
 * @param ipv4 Pointer to allocated IPv4 struct is returned to caller.
 *
 * @return 0 if ok, <0 if error
 */
int net_if_config_ipv4_get(struct net_if *iface,
			   struct net_if_ipv4 **ipv4);

/**
 * @brief Release network interface IPv4 config.
 *
 * @param iface Interface to use.
 *
 * @return 0 if ok, <0 if error
 */
int net_if_config_ipv4_put(struct net_if *iface);

/**
 * @brief Get IPv4 time-to-live value specified for a given interface
 *
 * @param iface Network interface
 *
 * @return Time-to-live
 */
uint8_t net_if_ipv4_get_ttl(struct net_if *iface);

/**
 * @brief Set IPv4 time-to-live value specified to a given interface
 *
 * @param iface Network interface
 * @param ttl Time-to-live value
 */
void net_if_ipv4_set_ttl(struct net_if *iface, uint8_t ttl);

/**
 * @brief Get IPv4 multicast time-to-live value specified for a given interface
 *
 * @param iface Network interface
 *
 * @return Time-to-live
 */
uint8_t net_if_ipv4_get_mcast_ttl(struct net_if *iface);

/**
 * @brief Set IPv4 multicast time-to-live value specified to a given interface
 *
 * @param iface Network interface
 * @param ttl Time-to-live value
 */
void net_if_ipv4_set_mcast_ttl(struct net_if *iface, uint8_t ttl);

/**
 * @brief Check if this IPv4 address belongs to one of the interfaces.
 *
 * @param addr IPv4 address
 * @param iface Interface is returned
 *
 * @return Pointer to interface address, NULL if not found.
 */
struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **iface);

/**
 * @brief Add a IPv4 address to an interface
 *
 * @param iface Network interface
 * @param addr IPv4 address
 * @param addr_type IPv4 address type
 * @param vlifetime Validity time for this address
 *
 * @return Pointer to interface address, NULL if cannot be added
 */
struct net_if_addr *net_if_ipv4_addr_add(struct net_if *iface,
					 const struct in_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime);

/**
 * @brief Remove a IPv4 address from an interface
 *
 * @param iface Network interface
 * @param addr IPv4 address
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv4_addr_rm(struct net_if *iface, const struct in_addr *addr);

/**
 * @brief Check if this IPv4 address belongs to one of the interface indices.
 *
 * @param addr IPv4 address
 *
 * @return >0 if address was found in given network interface index,
 * all other values mean address was not found
 */
__syscall int net_if_ipv4_addr_lookup_by_index(const struct in_addr *addr);

/**
 * @brief Add a IPv4 address to an interface by network interface index
 *
 * @param index Network interface index
 * @param addr IPv4 address
 * @param addr_type IPv4 address type
 * @param vlifetime Validity time for this address
 *
 * @return True if ok, false if the address could not be added
 */
__syscall bool net_if_ipv4_addr_add_by_index(int index,
					     const struct in_addr *addr,
					     enum net_addr_type addr_type,
					     uint32_t vlifetime);

/**
 * @brief Remove a IPv4 address from an interface by interface index
 *
 * @param index Network interface index
 * @param addr IPv4 address
 *
 * @return True if successfully removed, false otherwise
 */
__syscall bool net_if_ipv4_addr_rm_by_index(int index,
					    const struct in_addr *addr);

/**
 * @brief Go through all IPv4 addresses on a network interface and call callback
 * for each used address.
 *
 * @param iface Pointer to the network interface
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_if_ipv4_addr_foreach(struct net_if *iface, net_if_ip_addr_cb_t cb,
			      void *user_data);

/**
 * @brief Add a IPv4 multicast address to an interface
 *
 * @param iface Network interface
 * @param addr IPv4 multicast address
 *
 * @return Pointer to interface multicast address, NULL if cannot be added
 */
struct net_if_mcast_addr *net_if_ipv4_maddr_add(struct net_if *iface,
						const struct in_addr *addr);

/**
 * @brief Remove an IPv4 multicast address from an interface
 *
 * @param iface Network interface
 * @param addr IPv4 multicast address
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv4_maddr_rm(struct net_if *iface, const struct in_addr *addr);

/**
 * @brief Go through all IPv4 multicast addresses on a network interface and call
 * callback for each used address.
 *
 * @param iface Pointer to the network interface
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_if_ipv4_maddr_foreach(struct net_if *iface, net_if_ip_maddr_cb_t cb,
			       void *user_data);

/**
 * @brief Check if this IPv4 multicast address belongs to a specific interface
 * or one of the interfaces.
 *
 * @param addr IPv4 address
 * @param iface If *iface is null, then pointer to interface is returned,
 * otherwise the *iface value needs to be matched.
 *
 * @return Pointer to interface multicast address, NULL if not found.
 */
struct net_if_mcast_addr *net_if_ipv4_maddr_lookup(const struct in_addr *addr,
						   struct net_if **iface);

/**
 * @brief Mark a given multicast address to be joined.
 *
 * @param iface Network interface the address belongs to
 * @param addr IPv4 multicast address
 */
void net_if_ipv4_maddr_join(struct net_if *iface,
			    struct net_if_mcast_addr *addr);

/**
 * @brief Check if given multicast address is joined or not.
 *
 * @param addr IPv4 multicast address
 *
 * @return True if address is joined, False otherwise.
 */
static inline bool net_if_ipv4_maddr_is_joined(struct net_if_mcast_addr *addr)
{
	if (addr == NULL) {
		return false;
	}

	return addr->is_joined;
}

/**
 * @brief Mark a given multicast address to be left.
 *
 * @param iface Network interface the address belongs to
 * @param addr IPv4 multicast address
 */
void net_if_ipv4_maddr_leave(struct net_if *iface,
			     struct net_if_mcast_addr *addr);

/**
 * @brief Get the IPv4 address of the given router
 * @param router a network router
 *
 * @return pointer to the IPv4 address, or NULL if none
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
static inline struct in_addr *net_if_router_ipv4(struct net_if_router *router)
{
	if (router == NULL) {
		return NULL;
	}

	return &router->address.in_addr;
}
#else
static inline struct in_addr *net_if_router_ipv4(struct net_if_router *router)
{
	static struct in_addr addr;

	ARG_UNUSED(router);

	return &addr;
}
#endif

/**
 * @brief Check if IPv4 address is one of the routers configured
 * in the system.
 *
 * @param iface Network interface
 * @param addr IPv4 address
 *
 * @return Pointer to router information, NULL if cannot be found
 */
struct net_if_router *net_if_ipv4_router_lookup(struct net_if *iface,
						const struct in_addr *addr);

/**
 * @brief Find default router for this IPv4 address.
 *
 * @param iface Network interface. This can be NULL in which case we
 * go through all the network interfaces to find a suitable router.
 * @param addr IPv4 address
 *
 * @return Pointer to router information, NULL if cannot be found
 */
struct net_if_router *net_if_ipv4_router_find_default(struct net_if *iface,
						      const struct in_addr *addr);
/**
 * @brief Add IPv4 router to the system.
 *
 * @param iface Network interface
 * @param addr IPv4 address
 * @param is_default Is this router the default one
 * @param router_lifetime Lifetime of the router
 *
 * @return Pointer to router information, NULL if could not be added
 */
struct net_if_router *net_if_ipv4_router_add(struct net_if *iface,
					     const struct in_addr *addr,
					     bool is_default,
					     uint16_t router_lifetime);

/**
 * @brief Remove IPv4 router from the system.
 *
 * @param router Router information.
 *
 * @return True if successfully removed, false otherwise
 */
bool net_if_ipv4_router_rm(struct net_if_router *router);

/**
 * @brief Check if the given IPv4 address belongs to local subnet.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr IPv4 address
 *
 * @return True if address is part of local subnet, false otherwise.
 */
bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
			       const struct in_addr *addr);

/**
 * @brief Check if the given IPv4 address is a broadcast address.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr IPv4 address, this should be in network byte order
 *
 * @return True if address is a broadcast address, false otherwise.
 */
bool net_if_ipv4_is_addr_bcast(struct net_if *iface,
			       const struct in_addr *addr);

/**
 * @brief Get a network interface that should be used when sending
 * IPv4 network data to destination.
 *
 * @param dst IPv4 destination address
 *
 * @return Pointer to network interface to use, NULL if no suitable interface
 * could be found.
 */
#if defined(CONFIG_NET_IPV4)
struct net_if *net_if_ipv4_select_src_iface(const struct in_addr *dst);
#else
static inline struct net_if *net_if_ipv4_select_src_iface(
	const struct in_addr *dst)
{
	ARG_UNUSED(dst);

	return NULL;
}
#endif

/**
 * @brief Get a network interface that should be used when sending
 * IPv4 network data to destination. Also return the source IPv4 address from
 * that network interface.
 *
 * @param dst IPv4 destination address
 * @param src_addr IPv4 source address. This can be set to NULL if the source
 *                 address is not needed.
 *
 * @return Pointer to network interface to use, NULL if no suitable interface
 * could be found.
 */
#if defined(CONFIG_NET_IPV4)
struct net_if *net_if_ipv4_select_src_iface_addr(const struct in_addr *dst,
						 const struct in_addr **src_addr);
#else
static inline struct net_if *net_if_ipv4_select_src_iface_addr(
	const struct in_addr *dst, const struct in_addr **src_addr)
{
	ARG_UNUSED(dst);
	ARG_UNUSED(src_addr);

	return NULL;
}
#endif /* CONFIG_NET_IPV4 */

/**
 * @brief Get a IPv4 source address that should be used when sending
 * network data to destination.
 *
 * @param iface Interface to use when sending the packet.
 * If the interface is not known, then NULL can be given.
 * @param dst IPv4 destination address
 *
 * @return Pointer to IPv4 address to use, NULL if no IPv4 address
 * could be found.
 */
#if defined(CONFIG_NET_IPV4)
const struct in_addr *net_if_ipv4_select_src_addr(struct net_if *iface,
						  const struct in_addr *dst);
#else
static inline const struct in_addr *net_if_ipv4_select_src_addr(
	struct net_if *iface, const struct in_addr *dst)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(dst);

	return NULL;
}
#endif

/**
 * @brief Get a IPv4 link local address in a given state.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr_state IPv4 address state (preferred, tentative, deprecated)
 *
 * @return Pointer to link local IPv4 address, NULL if no proper IPv4 address
 * could be found.
 */
struct in_addr *net_if_ipv4_get_ll(struct net_if *iface,
				   enum net_addr_state addr_state);

/**
 * @brief Get a IPv4 global address in a given state.
 *
 * @param iface Interface to use. Must be a valid pointer to an interface.
 * @param addr_state IPv4 address state (preferred, tentative, deprecated)
 *
 * @return Pointer to link local IPv4 address, NULL if no proper IPv4 address
 * could be found.
 */
struct in_addr *net_if_ipv4_get_global_addr(struct net_if *iface,
					    enum net_addr_state addr_state);

/**
 * @brief Get IPv4 netmask related to an address of an interface.
 *
 * @param iface Interface to use.
 * @param addr IPv4 address to check.
 *
 * @return The netmask set on the interface related to the give address,
 *         unspecified address if not found.
 */
struct in_addr net_if_ipv4_get_netmask_by_addr(struct net_if *iface,
					       const struct in_addr *addr);

/**
 * @brief Get IPv4 netmask of an interface.
 *
 * @deprecated Use net_if_ipv4_get_netmask_by_addr() instead.
 *
 * @param iface Interface to use.
 *
 * @return The netmask set on the interface, unspecified address if not found.
 */
__deprecated struct in_addr net_if_ipv4_get_netmask(struct net_if *iface);

/**
 * @brief Set IPv4 netmask for an interface.
 *
 * @deprecated Use net_if_ipv4_set_netmask_by_addr() instead.
 *
 * @param iface Interface to use.
 * @param netmask IPv4 netmask
 */
__deprecated void net_if_ipv4_set_netmask(struct net_if *iface,
					  const struct in_addr *netmask);

/**
 * @brief Set IPv4 netmask for an interface index.
 *
 * @deprecated Use net_if_ipv4_set_netmask_by_addr() instead.
 *
 * @param index Network interface index
 * @param netmask IPv4 netmask
 *
 * @return True if netmask was added, false otherwise.
 */
__deprecated __syscall bool net_if_ipv4_set_netmask_by_index(int index,
							     const struct in_addr *netmask);

/**
 * @brief Set IPv4 netmask for an interface index for a given address.
 *
 * @param index Network interface index
 * @param addr IPv4 address related to this netmask
 * @param netmask IPv4 netmask
 *
 * @return True if netmask was added, false otherwise.
 */
__syscall bool net_if_ipv4_set_netmask_by_addr_by_index(int index,
							const struct in_addr *addr,
							const struct in_addr *netmask);

/**
 * @brief Set IPv4 netmask for an interface index for a given address.
 *
 * @param iface Network interface
 * @param addr IPv4 address related to this netmask
 * @param netmask IPv4 netmask
 *
 * @return True if netmask was added, false otherwise.
 */
bool net_if_ipv4_set_netmask_by_addr(struct net_if *iface,
				     const struct in_addr *addr,
				     const struct in_addr *netmask);

/**
 * @brief Get IPv4 gateway of an interface.
 *
 * @param iface Interface to use.
 *
 * @return The gateway set on the interface, unspecified address if not found.
 */
struct in_addr net_if_ipv4_get_gw(struct net_if *iface);

/**
 * @brief Set IPv4 gateway for an interface.
 *
 * @param iface Interface to use.
 * @param gw IPv4 address of an gateway
 */
void net_if_ipv4_set_gw(struct net_if *iface, const struct in_addr *gw);

/**
 * @brief Set IPv4 gateway for an interface index.
 *
 * @param index Network interface index
 * @param gw IPv4 address of an gateway
 *
 * @return True if gateway was added, false otherwise.
 */
__syscall bool net_if_ipv4_set_gw_by_index(int index, const struct in_addr *gw);

/**
 * @brief Get a network interface that should be used when sending
 * IPv6 or IPv4 network data to destination.
 *
 * @param dst IPv6 or IPv4 destination address
 *
 * @return Pointer to network interface to use. Note that the function
 * will return the default network interface if the best network interface
 * is not found.
 */
struct net_if *net_if_select_src_iface(const struct sockaddr *dst);

/**
 * @typedef net_if_link_callback_t
 * @brief Define callback that is called after a network packet
 *        has been sent.
 * @param iface A pointer to a struct net_if to which the net_pkt was sent to.
 * @param dst Link layer address of the destination where the network packet was sent.
 * @param status Send status, 0 is ok, < 0 error.
 */
typedef void (*net_if_link_callback_t)(struct net_if *iface,
				       struct net_linkaddr *dst,
				       int status);

/**
 * @brief Link callback handler struct.
 *
 * Stores the link callback information. Caller must make sure that
 * the variable pointed by this is valid during the lifetime of
 * registration. Typically this means that the variable cannot be
 * allocated from stack.
 */
struct net_if_link_cb {
	/** Node information for the slist. */
	sys_snode_t node;

	/** Link callback */
	net_if_link_callback_t cb;
};

/**
 * @brief Register a link callback.
 *
 * @param link Caller specified handler for the callback.
 * @param cb Callback to register.
 */
void net_if_register_link_cb(struct net_if_link_cb *link,
			     net_if_link_callback_t cb);

/**
 * @brief Unregister a link callback.
 *
 * @param link Caller specified handler for the callback.
 */
void net_if_unregister_link_cb(struct net_if_link_cb *link);

/**
 * @brief Call a link callback function.
 *
 * @param iface Network interface.
 * @param lladdr Destination link layer address
 * @param status 0 is ok, < 0 error
 */
void net_if_call_link_cb(struct net_if *iface, struct net_linkaddr *lladdr,
			 int status);

/** @cond INTERNAL_HIDDEN */

/* used to ensure encoding of checksum support in net_if.h and
 * ethernet.h is the same
 */
#define NET_IF_CHECKSUM_NONE_BIT			0
#define NET_IF_CHECKSUM_IPV4_HEADER_BIT			BIT(0)
#define NET_IF_CHECKSUM_IPV4_ICMP_BIT			BIT(1)
/* Space for future protocols and restrictions for IPV4 */
#define NET_IF_CHECKSUM_IPV6_HEADER_BIT			BIT(10)
#define NET_IF_CHECKSUM_IPV6_ICMP_BIT			BIT(11)
/* Space for future protocols and restrictions for IPV6 */
#define NET_IF_CHECKSUM_TCP_BIT				BIT(21)
#define NET_IF_CHECKSUM_UDP_BIT				BIT(22)

/** @endcond */

/**
 * @brief Type of checksum for which support in the interface will be queried.
 */
enum net_if_checksum_type {
	/** Interface supports IP version 4 header checksum calculation */
	NET_IF_CHECKSUM_IPV4_HEADER = NET_IF_CHECKSUM_IPV4_HEADER_BIT,
	/** Interface supports checksum calculation for TCP payload in IPv4 */
	NET_IF_CHECKSUM_IPV4_TCP    = NET_IF_CHECKSUM_IPV4_HEADER_BIT |
				      NET_IF_CHECKSUM_TCP_BIT,
	/** Interface supports checksum calculation for UDP payload in IPv4 */
	NET_IF_CHECKSUM_IPV4_UDP    = NET_IF_CHECKSUM_IPV4_HEADER_BIT |
				      NET_IF_CHECKSUM_UDP_BIT,
	/** Interface supports checksum calculation for ICMP4 payload in IPv4 */
	NET_IF_CHECKSUM_IPV4_ICMP   = NET_IF_CHECKSUM_IPV4_ICMP_BIT,
	/** Interface supports IP version 6 header checksum calculation */
	NET_IF_CHECKSUM_IPV6_HEADER = NET_IF_CHECKSUM_IPV6_HEADER_BIT,
	/** Interface supports checksum calculation for TCP payload in IPv6 */
	NET_IF_CHECKSUM_IPV6_TCP    = NET_IF_CHECKSUM_IPV6_HEADER_BIT |
				      NET_IF_CHECKSUM_TCP_BIT,
	/** Interface supports checksum calculation for UDP payload in IPv6 */
	NET_IF_CHECKSUM_IPV6_UDP    = NET_IF_CHECKSUM_IPV6_HEADER_BIT |
				      NET_IF_CHECKSUM_UDP_BIT,
	/** Interface supports checksum calculation for ICMP6 payload in IPv6 */
	NET_IF_CHECKSUM_IPV6_ICMP   = NET_IF_CHECKSUM_IPV6_ICMP_BIT
};

/**
 * @brief Check if received network packet checksum calculation can be avoided
 * or not. For example many ethernet devices support network packet offloading
 * in which case the IP stack does not need to calculate the checksum.
 *
 * @param iface Network interface
 * @param chksum_type L3 and/or L4 protocol for which to compute checksum
 *
 * @return True if checksum needs to be calculated, false otherwise.
 */
bool net_if_need_calc_rx_checksum(struct net_if *iface,
				  enum net_if_checksum_type chksum_type);

/**
 * @brief Check if network packet checksum calculation can be avoided or not
 * when sending the packet. For example many ethernet devices support network
 * packet offloading in which case the IP stack does not need to calculate the
 * checksum.
 *
 * @param iface Network interface
 * @param chksum_type L3 and/or L4 protocol for which to compute checksum
 *
 * @return True if checksum needs to be calculated, false otherwise.
 */
bool net_if_need_calc_tx_checksum(struct net_if *iface,
				  enum net_if_checksum_type chksum_type);

/**
 * @brief Get interface according to index
 *
 * @details This is a syscall only to provide access to the object for purposes
 *          of assigning permissions.
 *
 * @param index Interface index
 *
 * @return Pointer to interface or NULL if not found.
 */
__syscall struct net_if *net_if_get_by_index(int index);

/**
 * @brief Get interface index according to pointer
 *
 * @param iface Pointer to network interface
 *
 * @return Interface index
 */
int net_if_get_by_iface(struct net_if *iface);

/**
 * @typedef net_if_cb_t
 * @brief Callback used while iterating over network interfaces
 *
 * @param iface Pointer to current network interface
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_if_cb_t)(struct net_if *iface, void *user_data);

/**
 * @brief Go through all the network interfaces and call callback
 * for each interface.
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_if_foreach(net_if_cb_t cb, void *user_data);

/**
 * @brief Bring interface up
 *
 * @param iface Pointer to network interface
 *
 * @return 0 on success
 */
int net_if_up(struct net_if *iface);

/**
 * @brief Check if interface is up and running.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface is up, False if it is down.
 */
static inline bool net_if_is_up(struct net_if *iface)
{
	if (iface == NULL) {
		return false;
	}

	return net_if_flag_is_set(iface, NET_IF_UP) &&
	       net_if_flag_is_set(iface, NET_IF_RUNNING);
}

/**
 * @brief Bring interface down
 *
 * @param iface Pointer to network interface
 *
 * @return 0 on success
 */
int net_if_down(struct net_if *iface);

/**
 * @brief Check if interface was brought up by the administrator.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface is admin up, false otherwise.
 */
static inline bool net_if_is_admin_up(struct net_if *iface)
{
	if (iface == NULL) {
		return false;
	}

	return net_if_flag_is_set(iface, NET_IF_UP);
}

/**
 * @brief Underlying network device has detected the carrier (cable connected).
 *
 * @details The function should be used by the respective network device driver
 *          or L2 implementation to update its state on a network interface.
 *
 * @param iface Pointer to network interface
 */
void net_if_carrier_on(struct net_if *iface);

/**
 * @brief Underlying network device has lost the carrier (cable disconnected).
 *
 * @details The function should be used by the respective network device driver
 *          or L2 implementation to update its state on a network interface.
 *
 * @param iface Pointer to network interface
 */
void net_if_carrier_off(struct net_if *iface);

/**
 * @brief Check if carrier is present on network device.
 *
 * @param iface Pointer to network interface
 *
 * @return True if carrier is present, false otherwise.
 */
static inline bool net_if_is_carrier_ok(struct net_if *iface)
{
	if (iface == NULL) {
		return false;
	}

	return net_if_flag_is_set(iface, NET_IF_LOWER_UP);
}

/**
 * @brief Mark interface as dormant. Dormant state indicates that the interface
 *        is not ready to pass packets yet, but is waiting for some event
 *        (for example Wi-Fi network association).
 *
 * @details The function should be used by the respective network device driver
 *          or L2 implementation to update its state on a network interface.
 *
 * @param iface Pointer to network interface
 */
void net_if_dormant_on(struct net_if *iface);

/**
 * @brief Mark interface as not dormant.
 *
 * @details The function should be used by the respective network device driver
 *          or L2 implementation to update its state on a network interface.
 *
 * @param iface Pointer to network interface
 */
void net_if_dormant_off(struct net_if *iface);

/**
 * @brief Check if the interface is dormant.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface is dormant, false otherwise.
 */
static inline bool net_if_is_dormant(struct net_if *iface)
{
	if (iface == NULL) {
		return false;
	}

	return net_if_flag_is_set(iface, NET_IF_DORMANT);
}

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_NATIVE)
/**
 * @typedef net_if_timestamp_callback_t
 * @brief Define callback that is called after a network packet
 *        has been timestamped.
 * @param "struct net_pkt *pkt" A pointer on a struct net_pkt which has
 *        been timestamped after being sent.
 */
typedef void (*net_if_timestamp_callback_t)(struct net_pkt *pkt);

/**
 * @brief Timestamp callback handler struct.
 *
 * Stores the timestamp callback information. Caller must make sure that
 * the variable pointed by this is valid during the lifetime of
 * registration. Typically this means that the variable cannot be
 * allocated from stack.
 */
struct net_if_timestamp_cb {
	/** Node information for the slist. */
	sys_snode_t node;

	/** Packet for which the callback is needed.
	 *  A NULL value means all packets.
	 */
	struct net_pkt *pkt;

	/** Net interface for which the callback is needed.
	 *  A NULL value means all interfaces.
	 */
	struct net_if *iface;

	/** Timestamp callback */
	net_if_timestamp_callback_t cb;
};

/**
 * @brief Register a timestamp callback.
 *
 * @param handle Caller specified handler for the callback.
 * @param pkt Net packet for which the callback is registered. NULL for all
 * 	      packets.
 * @param iface Net interface for which the callback is. NULL for all
 *		interfaces.
 * @param cb Callback to register.
 */
void net_if_register_timestamp_cb(struct net_if_timestamp_cb *handle,
				  struct net_pkt *pkt,
				  struct net_if *iface,
				  net_if_timestamp_callback_t cb);

/**
 * @brief Unregister a timestamp callback.
 *
 * @param handle Caller specified handler for the callback.
 */
void net_if_unregister_timestamp_cb(struct net_if_timestamp_cb *handle);

/**
 * @brief Call a timestamp callback function.
 *
 * @param pkt Network buffer.
 */
void net_if_call_timestamp_cb(struct net_pkt *pkt);

/*
 * @brief Add timestamped TX buffer to be handled
 *
 * @param pkt Timestamped buffer
 */
void net_if_add_tx_timestamp(struct net_pkt *pkt);
#endif /* CONFIG_NET_PKT_TIMESTAMP */

/**
 * @brief Set network interface into promiscuous mode
 *
 * @details Note that not all network technologies will support this.
 *
 * @param iface Pointer to network interface
 *
 * @return 0 on success, <0 if error
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int net_if_set_promisc(struct net_if *iface);
#else
static inline int net_if_set_promisc(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENOTSUP;
}
#endif

/**
 * @brief Set network interface into normal mode
 *
 * @param iface Pointer to network interface
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
void net_if_unset_promisc(struct net_if *iface);
#else
static inline void net_if_unset_promisc(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Check if promiscuous mode is set or not.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface is in promisc mode,
 *         False if interface is not in promiscuous mode.
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
bool net_if_is_promisc(struct net_if *iface);
#else
static inline bool net_if_is_promisc(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return false;
}
#endif

/**
 * @brief Check if there are any pending TX network data for a given network
 *        interface.
 *
 * @param iface Pointer to network interface
 *
 * @return True if there are pending TX network packets for this network
 *         interface, False otherwise.
 */
static inline bool net_if_are_pending_tx_packets(struct net_if *iface)
{
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	return !!iface->tx_pending;
#else
	ARG_UNUSED(iface);

	return false;
#endif
}

#ifdef CONFIG_NET_POWER_MANAGEMENT
/**
 * @brief Suspend a network interface from a power management perspective
 *
 * @param iface Pointer to network interface
 *
 * @return 0 on success, or -EALREADY/-EBUSY as possible errors.
 */
int net_if_suspend(struct net_if *iface);

/**
 * @brief Resume a network interface from a power management perspective
 *
 * @param iface Pointer to network interface
 *
 * @return 0 on success, or -EALREADY as a possible error.
 */
int net_if_resume(struct net_if *iface);

/**
 * @brief Check if the network interface is suspended or not.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface is suspended, False otherwise.
 */
bool net_if_is_suspended(struct net_if *iface);
#endif /* CONFIG_NET_POWER_MANAGEMENT */

/**
 * @brief Check if the network interface supports Wi-Fi.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface supports Wi-Fi, False otherwise.
 */
bool net_if_is_wifi(struct net_if *iface);

/**
 * @brief Get first Wi-Fi network interface.
 *
 * @return Pointer to network interface, NULL if not found.
 */
struct net_if *net_if_get_first_wifi(void);

/**
 * @brief Get Wi-Fi network station interface.
 *
 * @return Pointer to network interface, NULL if not found.
 */
struct net_if *net_if_get_wifi_sta(void);

/**
 * @brief Get first Wi-Fi network Soft-AP interface.
 *
 * @return Pointer to network interface, NULL if not found.
 */
struct net_if *net_if_get_wifi_sap(void);

/**
 * @brief Get network interface name.
 *
 * @details If interface name support is not enabled, empty string is returned.
 *
 * @param iface Pointer to network interface
 * @param buf User supplied buffer
 * @param len Length of the user supplied buffer
 *
 * @return Length of the interface name copied to buf,
 *         -EINVAL if invalid parameters,
 *         -ERANGE if name cannot be copied to the user supplied buffer,
 *         -ENOTSUP if interface name support is disabled,
 */
int net_if_get_name(struct net_if *iface, char *buf, int len);

/**
 * @brief Set network interface name.
 *
 * @details Normally this function is not needed to call as the system
 *          will automatically assign a name to the network interface.
 *
 * @param iface Pointer to network interface
 * @param buf User supplied name
 *
 * @return 0 name is set correctly
 *         -ENOTSUP interface name support is disabled
 *         -EINVAL if invalid parameters are given,
 *         -ENAMETOOLONG if name is too long
 */
int net_if_set_name(struct net_if *iface, const char *buf);

/**
 * @brief Get interface index according to its name
 *
 * @param name Name of the network interface
 *
 * @return Interface index
 */
int net_if_get_by_name(const char *name);

/** @cond INTERNAL_HIDDEN */
struct net_if_api {
	void (*init)(struct net_if *iface);
};

#define NET_IF_DHCPV4_INIT						\
	IF_ENABLED(UTIL_AND(IS_ENABLED(CONFIG_NET_DHCPV4),		\
			    IS_ENABLED(CONFIG_NET_NATIVE_IPV4)),	\
		   (.dhcpv4.state = NET_DHCPV4_DISABLED,))

#define NET_IF_DHCPV6_INIT						\
	IF_ENABLED(UTIL_AND(IS_ENABLED(CONFIG_NET_DHCPV6),		\
			    IS_ENABLED(CONFIG_NET_NATIVE_IPV6)),	\
		   (.dhcpv6.state = NET_DHCPV6_DISABLED,))

#define NET_IF_CONFIG_INIT				\
	.config = {					\
		IF_ENABLED(CONFIG_NET_IP, (.ip = {},))  \
		NET_IF_DHCPV4_INIT			\
		NET_IF_DHCPV6_INIT			\
	}

#define NET_PROMETHEUS_GET_COLLECTOR_NAME(dev_id, sfx)			\
	net_stats_##dev_id##_##sfx##_collector
#define NET_PROMETHEUS_INIT(dev_id, sfx)				\
	IF_ENABLED(CONFIG_NET_STATISTICS_VIA_PROMETHEUS,		\
		   (.collector = &NET_PROMETHEUS_GET_COLLECTOR_NAME(dev_id, sfx),))

#define NET_IF_GET_NAME(dev_id, sfx) __net_if_##dev_id##_##sfx
#define NET_IF_DEV_GET_NAME(dev_id, sfx) __net_if_dev_##dev_id##_##sfx

#define NET_IF_GET(dev_id, sfx)						\
	((struct net_if *)&NET_IF_GET_NAME(dev_id, sfx))

#if defined(CONFIG_NET_STATISTICS_VIA_PROMETHEUS)
extern int net_stats_prometheus_scrape(struct prometheus_collector *collector,
				       struct prometheus_metric *metric,
				       void *user_data);
#endif /* CONFIG_NET_STATISTICS_VIA_PROMETHEUS */

#define NET_IF_INIT(dev_id, sfx, _l2, _mtu, _num_configs)		\
	static STRUCT_SECTION_ITERABLE(net_if_dev,			\
				NET_IF_DEV_GET_NAME(dev_id, sfx)) = {	\
		.dev = &(DEVICE_NAME_GET(dev_id)),			\
		.l2 = &(NET_L2_GET_NAME(_l2)),				\
		.l2_data = &(NET_L2_GET_DATA(dev_id, sfx)),		\
		.mtu = _mtu,						\
		.flags = {BIT(NET_IF_LOWER_UP)},			\
	};								\
	static Z_DECL_ALIGN(struct net_if)				\
		       NET_IF_GET_NAME(dev_id, sfx)[_num_configs]	\
		       __used __noasan __in_section(_net_if, static,	\
					   dev_id) = {			\
		[0 ... (_num_configs - 1)] = {				\
			.if_dev = &(NET_IF_DEV_GET_NAME(dev_id, sfx)),	\
			NET_IF_CONFIG_INIT				\
		}							\
	};								\
	IF_ENABLED(CONFIG_NET_STATISTICS_VIA_PROMETHEUS,		\
		   (static PROMETHEUS_COLLECTOR_DEFINE(			\
			   NET_PROMETHEUS_GET_COLLECTOR_NAME(dev_id,	\
							     sfx),	\
			   net_stats_prometheus_scrape,			\
			   NET_IF_GET(dev_id, sfx));			\
		    NET_STATS_PROMETHEUS(NET_IF_GET(dev_id, sfx),	\
					 dev_id, sfx);))

#define NET_IF_OFFLOAD_INIT(dev_id, sfx, _mtu)				\
	static STRUCT_SECTION_ITERABLE(net_if_dev,			\
				NET_IF_DEV_GET_NAME(dev_id, sfx)) = {	\
		.dev = &(DEVICE_NAME_GET(dev_id)),			\
		.mtu = _mtu,						\
		.l2 = &(NET_L2_GET_NAME(OFFLOADED_NETDEV)),		\
		.flags = {BIT(NET_IF_LOWER_UP)},			\
	};								\
	static Z_DECL_ALIGN(struct net_if)				\
		NET_IF_GET_NAME(dev_id, sfx)[NET_IF_MAX_CONFIGS]	\
		       __used __noasan __in_section(_net_if, static,	\
					   dev_id) = {			\
		[0 ... (NET_IF_MAX_CONFIGS - 1)] = {			\
			.if_dev = &(NET_IF_DEV_GET_NAME(dev_id, sfx)),	\
			NET_IF_CONFIG_INIT				\
		}							\
	};								\
	IF_ENABLED(CONFIG_NET_STATISTICS_VIA_PROMETHEUS,		\
		   (static PROMETHEUS_COLLECTOR_DEFINE(			\
			   NET_PROMETHEUS_GET_COLLECTOR_NAME(dev_id,	\
							     sfx),	\
			   net_stats_prometheus_scrape,			\
			   NET_IF_GET(dev_id, sfx));			\
		    NET_STATS_PROMETHEUS(NET_IF_GET(dev_id, sfx),	\
					 dev_id, sfx);))


/** @endcond */

/* Network device initialization macros */

#define Z_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, instance,	\
				   init_fn, pm, data, config, prio,	\
				   api, l2, l2_ctx_type, mtu)		\
	Z_DEVICE_STATE_DEFINE(dev_id);					\
	Z_DEVICE_DEFINE(node_id, dev_id, name, init_fn, NULL,		\
			Z_DEVICE_DT_FLAGS(node_id), pm, data,		\
			config, POST_KERNEL, prio, api,			\
			&Z_DEVICE_STATE_NAME(dev_id));			\
	NET_L2_DATA_INIT(dev_id, instance, l2_ctx_type);		\
	NET_IF_INIT(dev_id, instance, l2, mtu, NET_IF_MAX_CONFIGS)

#define Z_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			  config, prio, api, l2, l2_ctx_type, mtu)	\
	Z_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, 0, init_fn,	\
				   pm, data, config, prio, api, l2,	\
				   l2_ctx_type, mtu)

/**
 * @brief Create a network interface and bind it to network device.
 *
 * @param dev_id Network device id.
 * @param name The name this instance of the driver exposes to
 * the system.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param l2 Network L2 layer for this network interface.
 * @param l2_ctx_type Type of L2 context data.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_INIT(dev_id, name, init_fn, pm, data, config, prio,	\
			api, l2, l2_ctx_type, mtu)			\
	Z_NET_DEVICE_INIT(DT_INVALID_NODE, dev_id, name, init_fn, pm,	\
			  data, config, prio, api, l2, l2_ctx_type, mtu)

/**
 * @brief Like NET_DEVICE_INIT but taking metadata from a devicetree node.
 * Create a network interface and bind it to network device.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param l2 Network L2 layer for this network interface.
 * @param l2_ctx_type Type of L2 context data.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_DT_DEFINE(node_id, init_fn, pm, data,		\
			     config, prio, api, l2, l2_ctx_type, mtu)	\
	Z_NET_DEVICE_INIT(node_id, Z_DEVICE_DT_DEV_ID(node_id),	\
			  DEVICE_DT_NAME(node_id), init_fn, pm, data,	\
			  config, prio, api, l2, l2_ctx_type, mtu)

/**
 * @brief Like NET_DEVICE_DT_DEFINE for an instance of a DT_DRV_COMPAT compatible
 *
 * @param inst instance number.  This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to NET_DEVICE_DT_DEFINE.
 *
 * @param ... other parameters as expected by NET_DEVICE_DT_DEFINE.
 */
#define NET_DEVICE_DT_INST_DEFINE(inst, ...) \
	NET_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Create multiple network interfaces and bind them to network device.
 * If your network device needs more than one instance of a network interface,
 * use this macro below and provide a different instance suffix each time
 * (0, 1, 2, ... or a, b, c ... whatever works for you)
 *
 * @param dev_id Network device id.
 * @param name The name this instance of the driver exposes to
 * the system.
 * @param instance Instance identifier.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param l2 Network L2 layer for this network interface.
 * @param l2_ctx_type Type of L2 context data.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_INIT_INSTANCE(dev_id, name, instance, init_fn, pm,	\
				 data, config, prio, api, l2,		\
				 l2_ctx_type, mtu)			\
	Z_NET_DEVICE_INIT_INSTANCE(DT_INVALID_NODE, dev_id, name,	\
				   instance, init_fn, pm, data, config,	\
				   prio, api, l2, l2_ctx_type, mtu)

/**
 * @brief Like NET_DEVICE_OFFLOAD_INIT but taking metadata from a devicetree.
 * Create multiple network interfaces and bind them to network device.
 * If your network device needs more than one instance of a network interface,
 * use this macro below and provide a different instance suffix each time
 * (0, 1, 2, ... or a, b, c ... whatever works for you)
 *
 * @param node_id The devicetree node identifier.
 * @param instance Instance identifier.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param l2 Network L2 layer for this network interface.
 * @param l2_ctx_type Type of L2 context data.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_DT_DEFINE_INSTANCE(node_id, instance, init_fn, pm,	\
				      data, config, prio, api, l2,	\
				      l2_ctx_type, mtu)			\
	Z_NET_DEVICE_INIT_INSTANCE(node_id,				\
				   Z_DEVICE_DT_DEV_ID(node_id),		\
				   DEVICE_DT_NAME(node_id), instance,	\
				   init_fn, pm, data, config, prio,	\
				   api,	l2, l2_ctx_type, mtu)

/**
 * @brief Like NET_DEVICE_DT_DEFINE_INSTANCE for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number.  This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to NET_DEVICE_DT_DEFINE_INSTANCE.
 *
 * @param ... other parameters as expected by NET_DEVICE_DT_DEFINE_INSTANCE.
 */
#define NET_DEVICE_DT_INST_DEFINE_INSTANCE(inst, ...) \
	NET_DEVICE_DT_DEFINE_INSTANCE(DT_DRV_INST(inst), __VA_ARGS__)

#define Z_NET_DEVICE_OFFLOAD_INIT(node_id, dev_id, name, init_fn, pm,	\
				  data, config, prio, api, mtu)		\
	Z_DEVICE_STATE_DEFINE(dev_id);					\
	Z_DEVICE_DEFINE(node_id, dev_id, name, init_fn,	NULL,		\
			Z_DEVICE_DT_FLAGS(node_id), pm, data,		\
			config, POST_KERNEL, prio, api,			\
			&Z_DEVICE_STATE_NAME(dev_id));			\
	NET_IF_OFFLOAD_INIT(dev_id, 0, mtu)

/**
 * @brief Create a offloaded network interface and bind it to network device.
 * The offloaded network interface is implemented by a device vendor HAL or
 * similar.
 *
 * @param dev_id Network device id.
 * @param name The name this instance of the driver exposes to
 * the system.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_OFFLOAD_INIT(dev_id, name, init_fn, pm, data,	\
				config,	prio, api, mtu)			\
	Z_NET_DEVICE_OFFLOAD_INIT(DT_INVALID_NODE, dev_id, name,	\
				  init_fn, pm, data, config, prio, api,	\
				  mtu)

/**
 * @brief Like NET_DEVICE_OFFLOAD_INIT but taking metadata from a devicetree
 * node. Create a offloaded network interface and bind it to network device.
 * The offloaded network interface is implemented by a device vendor HAL or
 * similar.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define NET_DEVICE_DT_OFFLOAD_DEFINE(node_id, init_fn, pm, data,	\
				     config, prio, api, mtu)		\
	Z_NET_DEVICE_OFFLOAD_INIT(node_id, Z_DEVICE_DT_DEV_ID(node_id), \
				  DEVICE_DT_NAME(node_id), init_fn, pm, \
				  data, config,	prio, api, mtu)

/**
 * @brief Like NET_DEVICE_DT_OFFLOAD_DEFINE for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number.  This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to NET_DEVICE_DT_OFFLOAD_DEFINE.
 *
 * @param ... other parameters as expected by NET_DEVICE_DT_OFFLOAD_DEFINE.
 */
#define NET_DEVICE_DT_INST_OFFLOAD_DEFINE(inst, ...) \
	NET_DEVICE_DT_OFFLOAD_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Count the number of network interfaces.
 *
 * @param[out] _dst Pointer to location where result is written.
 */
#define NET_IFACE_COUNT(_dst) \
		do {							\
			extern struct net_if _net_if_list_start[];	\
			extern struct net_if _net_if_list_end[];	\
			*(_dst) = ((uintptr_t)_net_if_list_end -	\
				   (uintptr_t)_net_if_list_start) /	\
				sizeof(struct net_if);			\
		} while (0)

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/net_if.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_IF_H_ */
