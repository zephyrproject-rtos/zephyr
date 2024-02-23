/** @file
 @brief Ethernet

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_ETHERNET_H_
#define ZEPHYR_INCLUDE_NET_ETHERNET_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#if defined(CONFIG_NET_LLDP)
#include <zephyr/net/lldp.h>
#endif

#include <zephyr/sys/util.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet_vlan.h>
#include <zephyr/net/ptp_time.h>

#if defined(CONFIG_NET_DSA)
#include <zephyr/net/dsa.h>
#endif

#if defined(CONFIG_NET_ETHERNET_BRIDGE)
#include <zephyr/net/ethernet_bridge.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ethernet support functions
 * @defgroup ethernet Ethernet Support Functions
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct net_eth_addr {
	uint8_t addr[6];
};

#define NET_ETH_HDR(pkt) ((struct net_eth_hdr *)net_pkt_data(pkt))

#define NET_ETH_PTYPE_ARP		0x0806
#define NET_ETH_PTYPE_IP		0x0800
#define NET_ETH_PTYPE_TSN		0x22f0 /* TSN (IEEE 1722) packet */
#define NET_ETH_PTYPE_IPV6		0x86dd
#define NET_ETH_PTYPE_VLAN		0x8100
#define NET_ETH_PTYPE_PTP		0x88f7
#define NET_ETH_PTYPE_LLDP		0x88cc
#define NET_ETH_PTYPE_ALL               0x0003 /* from linux/if_ether.h */
#define NET_ETH_PTYPE_ECAT		0x88a4
#define NET_ETH_PTYPE_EAPOL		0x888e
#define NET_ETH_PTYPE_IEEE802154	0x00F6 /* from linux/if_ether.h: IEEE802.15.4 frame */

#if !defined(ETH_P_ALL)
#define ETH_P_ALL	NET_ETH_PTYPE_ALL
#endif
#if !defined(ETH_P_IP)
#define ETH_P_IP	NET_ETH_PTYPE_IP
#endif
#if !defined(ETH_P_ARP)
#define ETH_P_ARP	NET_ETH_PTYPE_ARP
#endif
#if !defined(ETH_P_IPV6)
#define ETH_P_IPV6	NET_ETH_PTYPE_IPV6
#endif
#if !defined(ETH_P_8021Q)
#define ETH_P_8021Q	NET_ETH_PTYPE_VLAN
#endif
#if !defined(ETH_P_TSN)
#define ETH_P_TSN	NET_ETH_PTYPE_TSN
#endif
#if !defined(ETH_P_ECAT)
#define  ETH_P_ECAT	NET_ETH_PTYPE_ECAT
#endif
#if !defined(ETH_P_IEEE802154)
#define  ETH_P_IEEE802154 NET_ETH_PTYPE_IEEE802154
#endif

#define NET_ETH_MINIMAL_FRAME_SIZE	60
#define NET_ETH_MTU			1500

#if defined(CONFIG_NET_VLAN)
#define _NET_ETH_MAX_HDR_SIZE		(sizeof(struct net_eth_vlan_hdr))
#else
#define _NET_ETH_MAX_HDR_SIZE		(sizeof(struct net_eth_hdr))
#endif

#define _NET_ETH_MAX_FRAME_SIZE	(NET_ETH_MTU + _NET_ETH_MAX_HDR_SIZE)
/*
 * Extend the max frame size for DSA (KSZ8794) by one byte (to 1519) to
 * store tail tag.
 */
#if defined(CONFIG_NET_DSA)
#define NET_ETH_MAX_FRAME_SIZE (_NET_ETH_MAX_FRAME_SIZE + DSA_TAG_SIZE)
#define NET_ETH_MAX_HDR_SIZE (_NET_ETH_MAX_HDR_SIZE + DSA_TAG_SIZE)
#else
#define NET_ETH_MAX_FRAME_SIZE (_NET_ETH_MAX_FRAME_SIZE)
#define NET_ETH_MAX_HDR_SIZE (_NET_ETH_MAX_HDR_SIZE)
#endif

#define NET_ETH_VLAN_HDR_SIZE	4

/** @endcond */

/** Ethernet hardware capabilities */
enum ethernet_hw_caps {
	/** TX Checksum offloading supported for all of IPv4, UDP, TCP */
	ETHERNET_HW_TX_CHKSUM_OFFLOAD	= BIT(0),

	/** RX Checksum offloading supported for all of IPv4, UDP, TCP */
	ETHERNET_HW_RX_CHKSUM_OFFLOAD	= BIT(1),

	/** VLAN supported */
	ETHERNET_HW_VLAN		= BIT(2),

	/** Enabling/disabling auto negotiation supported */
	ETHERNET_AUTO_NEGOTIATION_SET	= BIT(3),

	/** 10 Mbits link supported */
	ETHERNET_LINK_10BASE_T		= BIT(4),

	/** 100 Mbits link supported */
	ETHERNET_LINK_100BASE_T		= BIT(5),

	/** 1 Gbits link supported */
	ETHERNET_LINK_1000BASE_T	= BIT(6),

	/** Changing duplex (half/full) supported */
	ETHERNET_DUPLEX_SET		= BIT(7),

	/** IEEE 802.1AS (gPTP) clock supported */
	ETHERNET_PTP			= BIT(8),

	/** IEEE 802.1Qav (credit-based shaping) supported */
	ETHERNET_QAV			= BIT(9),

	/** Promiscuous mode supported */
	ETHERNET_PROMISC_MODE		= BIT(10),

	/** Priority queues available */
	ETHERNET_PRIORITY_QUEUES	= BIT(11),

	/** MAC address filtering supported */
	ETHERNET_HW_FILTERING		= BIT(12),

	/** Link Layer Discovery Protocol supported */
	ETHERNET_LLDP			= BIT(13),

	/** VLAN Tag stripping */
	ETHERNET_HW_VLAN_TAG_STRIP	= BIT(14),

	/** DSA switch */
	ETHERNET_DSA_SLAVE_PORT	= BIT(15),
	ETHERNET_DSA_MASTER_PORT	= BIT(16),

	/** IEEE 802.1Qbv (scheduled traffic) supported */
	ETHERNET_QBV			= BIT(17),

	/** IEEE 802.1Qbu (frame preemption) supported */
	ETHERNET_QBU			= BIT(18),

	/** TXTIME supported */
	ETHERNET_TXTIME			= BIT(19),

	/** TX-Injection supported */
	ETHERNET_TXINJECTION_MODE	= BIT(20),
};

/** @cond INTERNAL_HIDDEN */

enum ethernet_config_type {
	ETHERNET_CONFIG_TYPE_AUTO_NEG,
	ETHERNET_CONFIG_TYPE_LINK,
	ETHERNET_CONFIG_TYPE_DUPLEX,
	ETHERNET_CONFIG_TYPE_MAC_ADDRESS,
	ETHERNET_CONFIG_TYPE_QAV_PARAM,
	ETHERNET_CONFIG_TYPE_QBV_PARAM,
	ETHERNET_CONFIG_TYPE_QBU_PARAM,
	ETHERNET_CONFIG_TYPE_TXTIME_PARAM,
	ETHERNET_CONFIG_TYPE_PROMISC_MODE,
	ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM,
	ETHERNET_CONFIG_TYPE_FILTER,
	ETHERNET_CONFIG_TYPE_PORTS_NUM,
	ETHERNET_CONFIG_TYPE_T1S_PARAM,
	ETHERNET_CONFIG_TYPE_TXINJECTION_MODE,
};

enum ethernet_qav_param_type {
	ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH,
	ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE,
	ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE,
	ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS,
	ETHERNET_QAV_PARAM_TYPE_STATUS,
};

enum ethernet_t1s_param_type {
	ETHERNET_T1S_PARAM_TYPE_PLCA_CONFIG,
};

/** @endcond */
struct ethernet_t1s_param {
	/** Type of T1S parameter */
	enum ethernet_t1s_param_type type;
	union {
		/* PLCA is the Physical Layer (PHY) Collision
		 * Avoidance technique employed with multidrop
		 * 10Base-T1S standard.
		 *
		 * The PLCA parameters are described in standard [1]
		 * as registers in memory map 4 (MMS = 4) (point 9.6).
		 *
		 * IDVER	(PLCA ID Version)
		 * CTRL0	(PLCA Control 0)
		 * CTRL1	(PLCA Control 1)
		 * STATUS	(PLCA Status)
		 * TOTMR	(PLCA TO Control)
		 * BURST	(PLCA Burst Control)
		 *
		 * Those registers are implemented by each OA TC6
		 * compliant vendor (like for e.g. LAN865x - e.g. [2]).
		 *
		 * Documents:
		 * [1] - "OPEN Alliance 10BASE-T1x MAC-PHY Serial
		 *       Interface" (ver. 1.1)
		 * [2] - "DS60001734C" - LAN865x data sheet
		 */
		struct {
			/** T1S PLCA enabled */
			bool enable;
			/** T1S PLCA node id range: 0 to 254 */
			uint8_t node_id;
			/** T1S PLCA node count range: 1 to 255 */
			uint8_t node_count;
			/** T1S PLCA burst count range: 0x0 to 0xFF */
			uint8_t burst_count;
			/** T1S PLCA burst timer */
			uint8_t burst_timer;
			/** T1S PLCA TO value */
			uint8_t to_timer;
		} plca;
	};
};

struct ethernet_qav_param {
	/** ID of the priority queue to use */
	int queue_id;
	/** Type of Qav parameter */
	enum ethernet_qav_param_type type;
	union {
		/** True if Qav is enabled for queue */
		bool enabled;
		/** Delta Bandwidth (percentage of bandwidth) */
		unsigned int delta_bandwidth;
		/** Idle Slope (bits per second) */
		unsigned int idle_slope;
		/** Oper Idle Slope (bits per second) */
		unsigned int oper_idle_slope;
		/** Traffic class the queue is bound to */
		unsigned int traffic_class;
	};
};

/** @cond INTERNAL_HIDDEN */

enum ethernet_qbv_param_type {
	ETHERNET_QBV_PARAM_TYPE_STATUS,
	ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST,
	ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN,
	ETHERNET_QBV_PARAM_TYPE_TIME,
};

enum ethernet_qbv_state_type {
	ETHERNET_QBV_STATE_TYPE_ADMIN,
	ETHERNET_QBV_STATE_TYPE_OPER,
};

enum ethernet_gate_state_operation {
	ETHERNET_SET_GATE_STATE,
	ETHERNET_SET_AND_HOLD_MAC_STATE,
	ETHERNET_SET_AND_RELEASE_MAC_STATE,
};

/** @endcond */

struct ethernet_qbv_param {
	/** Port id */
	int port_id;
	/** Type of Qbv parameter */
	enum ethernet_qbv_param_type type;
	/** What state (Admin/Oper) parameters are these */
	enum ethernet_qbv_state_type state;
	union {
		/** True if Qbv is enabled or not */
		bool enabled;

		struct {
			/** True = open, False = closed */
			bool gate_status[NET_TC_TX_COUNT];

			/** GateState operation */
			enum ethernet_gate_state_operation operation;

			/** Time interval ticks (nanoseconds) */
			uint32_t time_interval;

			/** Gate control list row */
			uint16_t row;
		} gate_control;

		/** Number of entries in gate control list */
		uint32_t gate_control_list_len;

		/* The time values are set in one go when type is set to
		 * ETHERNET_QBV_PARAM_TYPE_TIME
		 */
		struct {
			/** Base time */
			struct net_ptp_extended_time base_time;

			/** Cycle time */
			struct net_ptp_time cycle_time;

			/** Extension time (nanoseconds) */
			uint32_t extension_time;
		};
	};
};

/** @cond INTERNAL_HIDDEN */

enum ethernet_qbu_param_type {
	ETHERNET_QBU_PARAM_TYPE_STATUS,
	ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE,
	ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE,
	ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE,

	/* Some preemption settings are from Qbr spec. */
	ETHERNET_QBR_PARAM_TYPE_LINK_PARTNER_STATUS,
	ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE,
};

enum ethernet_qbu_preempt_status {
	ETHERNET_QBU_STATUS_EXPRESS,
	ETHERNET_QBU_STATUS_PREEMPTABLE
} __packed;

/** @endcond */

struct ethernet_qbu_param {
	/** Port id */
	int port_id;
	/** Type of Qbu parameter */
	enum ethernet_qbu_param_type type;
	union {
		/** Hold advance (nanoseconds) */
		uint32_t hold_advance;

		/** Release advance (nanoseconds) */
		uint32_t release_advance;

		/** sequence of framePreemptionAdminStatus values.
		 */
		enum ethernet_qbu_preempt_status
				frame_preempt_statuses[NET_TC_TX_COUNT];

		/** True if Qbu is enabled or not */
		bool enabled;

		/** Link partner status (from Qbr) */
		bool link_partner_status;

		/** Additional fragment size (from Qbr). The minimum non-final
		 * fragment size is (additional_fragment_size + 1) * 64 octets
		 */
		uint8_t additional_fragment_size : 2;
	};
};


/** @cond INTERNAL_HIDDEN */

enum ethernet_filter_type {
	ETHERNET_FILTER_TYPE_SRC_MAC_ADDRESS,
	ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS,
};

/** @endcond */

/** Types of Ethernet L2 */
enum ethernet_if_types {
	/** IEEE 802.3 Ethernet (default) */
	L2_ETH_IF_TYPE_ETHERNET,

	/** IEEE 802.11 Wi-Fi*/
	L2_ETH_IF_TYPE_WIFI,
} __packed;


struct ethernet_filter {
	/** Type of filter */
	enum ethernet_filter_type type;
	/** MAC address to filter */
	struct net_eth_addr mac_address;
	/** Set (true) or unset (false) the filter */
	bool set;
};

/** @cond INTERNAL_HIDDEN */

enum ethernet_txtime_param_type {
	ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES,
};

/** @endcond */

struct ethernet_txtime_param {
	/** Type of TXTIME parameter */
	enum ethernet_txtime_param_type type;
	/** Queue number for configuring TXTIME */
	int queue_id;
	/** Enable or disable TXTIME per queue */
	bool enable_txtime;
};

/** @cond INTERNAL_HIDDEN */
struct ethernet_config {
	union {
		bool auto_negotiation;
		bool full_duplex;
		bool promisc_mode;
		bool txinjection_mode;

		struct {
			bool link_10bt;
			bool link_100bt;
			bool link_1000bt;
		} l;

		struct net_eth_addr mac_address;

		struct ethernet_t1s_param t1s_param;
		struct ethernet_qav_param qav_param;
		struct ethernet_qbv_param qbv_param;
		struct ethernet_qbu_param qbu_param;
		struct ethernet_txtime_param txtime_param;

		int priority_queues_num;
		int ports_num;

		struct ethernet_filter filter;
	};
};
/** @endcond */

struct ethernet_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	/** Collect optional ethernet specific statistics. This pointer
	 * should be set by driver if statistics needs to be collected
	 * for that driver.
	 */
	struct net_stats_eth *(*get_stats)(const struct device *dev);
#endif

	/** Start the device */
	int (*start)(const struct device *dev);

	/** Stop the device */
	int (*stop)(const struct device *dev);

	/** Get the device capabilities */
	enum ethernet_hw_caps (*get_capabilities)(const struct device *dev);

	/** Set specific hardware configuration */
	int (*set_config)(const struct device *dev,
			  enum ethernet_config_type type,
			  const struct ethernet_config *config);

	/** Get hardware specific configuration */
	int (*get_config)(const struct device *dev,
			  enum ethernet_config_type type,
			  struct ethernet_config *config);

#if defined(CONFIG_NET_VLAN)
	/** The IP stack will call this function when a VLAN tag is enabled
	 * or disabled. If enable is set to true, then the VLAN tag was added,
	 * if it is false then the tag was removed. The driver can utilize
	 * this information if needed.
	 */
	int (*vlan_setup)(const struct device *dev, struct net_if *iface,
			  uint16_t tag, bool enable);
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK)
	/** Return ptp_clock device that is tied to this ethernet device */
	const struct device *(*get_ptp_clock)(const struct device *dev);
#endif /* CONFIG_PTP_CLOCK */

	/** Send a network packet */
	int (*send)(const struct device *dev, struct net_pkt *pkt);
};

/* Make sure that the network interface API is properly setup inside
 * Ethernet API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct ethernet_api, iface_api) == 0);

/** @cond INTERNAL_HIDDEN */
struct net_eth_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	uint16_t type;
} __packed;

struct ethernet_vlan {
	/** Network interface that has VLAN enabled */
	struct net_if *iface;

	/** VLAN tag */
	uint16_t tag;
};

#if defined(CONFIG_NET_VLAN_COUNT)
#define NET_VLAN_MAX_COUNT CONFIG_NET_VLAN_COUNT
#else
/* Even thou there are no VLAN support, the minimum count must be set to 1.
 */
#define NET_VLAN_MAX_COUNT 1
#endif

/** @endcond */

#if defined(CONFIG_NET_LLDP)
struct ethernet_lldp {
	/** Used for track timers */
	sys_snode_t node;

	/** LLDP Data Unit mandatory TLVs for the interface. */
	const struct net_lldpdu *lldpdu;

	/** LLDP Data Unit optional TLVs for the interface */
	const uint8_t *optional_du;

	/** Length of the optional Data Unit TLVs */
	size_t optional_len;

	/** Network interface that has LLDP supported. */
	struct net_if *iface;

	/** LLDP TX timer start time */
	int64_t tx_timer_start;

	/** LLDP TX timeout */
	uint32_t tx_timer_timeout;

	/** LLDP RX callback function */
	net_lldp_recv_cb_t cb;
};
#endif /* CONFIG_NET_LLDP */

enum ethernet_flags {
	ETH_CARRIER_UP,
};

/** Ethernet L2 context that is needed for VLAN */
struct ethernet_context {
	/** Flags representing ethernet state, which are accessed from multiple
	 * threads.
	 */
	atomic_t flags;

#if defined(CONFIG_NET_VLAN)
	struct ethernet_vlan vlan[NET_VLAN_MAX_COUNT];

	/** Array that will help when checking if VLAN is enabled for
	 * some specific network interface. Requires that VLAN count
	 * NET_VLAN_MAX_COUNT is not smaller than the actual number
	 * of network interfaces.
	 */
	ATOMIC_DEFINE(interfaces, NET_VLAN_MAX_COUNT);
#endif

#if defined(CONFIG_NET_ETHERNET_BRIDGE)
	struct eth_bridge_iface_context bridge;
#endif

	/** Carrier ON/OFF handler worker. This is used to create
	 * network interface UP/DOWN event when ethernet L2 driver
	 * notices carrier ON/OFF situation. We must not create another
	 * network management event from inside management handler thus
	 * we use worker thread to trigger the UP/DOWN event.
	 */
	struct k_work carrier_work;

	/** Network interface. */
	struct net_if *iface;

#if defined(CONFIG_NET_LLDP)
	struct ethernet_lldp lldp[NET_VLAN_MAX_COUNT];
#endif

	/**
	 * This tells what L2 features does ethernet support.
	 */
	enum net_l2_flags ethernet_l2_flags;

#if defined(CONFIG_NET_L2_PTP)
	/** The PTP port number for this network device. We need to store the
	 * port number here so that we do not need to fetch it for every
	 * incoming PTP packet.
	 */
	int port;
#endif

#if defined(CONFIG_NET_DSA)
	/** DSA RX callback function - for custom processing - like e.g.
	 * redirecting packets when MAC address is caught
	 */
	dsa_net_recv_cb_t dsa_recv_cb;

	/** Switch physical port number */
	uint8_t dsa_port_idx;

	/** DSA context pointer */
	struct dsa_context *dsa_ctx;

	/** Send a network packet via DSA master port */
	dsa_send_t dsa_send;
#endif

#if defined(CONFIG_NET_VLAN)
	/** Flag that tells whether how many VLAN tags are enabled for this
	 * context. The same information can be dug from the vlan array but
	 * this saves some time in RX path.
	 */
	int8_t vlan_enabled;
#endif

	/** Is network carrier up */
	bool is_net_carrier_up : 1;

	/** Is this context already initialized */
	bool is_init : 1;

	/** Types of Ethernet network interfaces */
	enum ethernet_if_types eth_if_type;
};

/**
 * @brief Initialize Ethernet L2 stack for a given interface
 *
 * @param iface A valid pointer to a network interface
 */
void ethernet_init(struct net_if *iface);

/** @cond INTERNAL_HIDDEN */

#define ETHERNET_L2_CTX_TYPE	struct ethernet_context

/* Separate header for VLAN as some of device interfaces might not
 * support VLAN.
 */
struct net_eth_vlan_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	struct {
		uint16_t tpid; /* tag protocol id  */
		uint16_t tci;  /* tag control info */
	} vlan;
	uint16_t type;
} __packed;


static inline bool net_eth_is_addr_broadcast(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0xff &&
	    addr->addr[1] == 0xff &&
	    addr->addr[2] == 0xff &&
	    addr->addr[3] == 0xff &&
	    addr->addr[4] == 0xff &&
	    addr->addr[5] == 0xff) {
		return true;
	}

	return false;
}

static inline bool net_eth_is_addr_unspecified(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0x00 &&
	    addr->addr[1] == 0x00 &&
	    addr->addr[2] == 0x00 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    addr->addr[5] == 0x00) {
		return true;
	}

	return false;
}

static inline bool net_eth_is_addr_multicast(struct net_eth_addr *addr)
{
#if defined(CONFIG_NET_IPV6)
	if (addr->addr[0] == 0x33 &&
	    addr->addr[1] == 0x33) {
		return true;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x00 &&
	    addr->addr[2] == 0x5e) {
		return true;
	}
#endif

	return false;
}

static inline bool net_eth_is_addr_group(struct net_eth_addr *addr)
{
	return addr->addr[0] & 0x01;
}

static inline bool net_eth_is_addr_valid(struct net_eth_addr *addr)
{
	return !net_eth_is_addr_unspecified(addr) && !net_eth_is_addr_group(addr);
}

static inline bool net_eth_is_addr_lldp_multicast(struct net_eth_addr *addr)
{
#if defined(CONFIG_NET_GPTP) || defined(CONFIG_NET_LLDP)
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x80 &&
	    addr->addr[2] == 0xc2 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    addr->addr[5] == 0x0e) {
		return true;
	}
#endif

	return false;
}

static inline bool net_eth_is_addr_ptp_multicast(struct net_eth_addr *addr)
{
#if defined(CONFIG_NET_GPTP)
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x1b &&
	    addr->addr[2] == 0x19 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    addr->addr[5] == 0x00) {
		return true;
	}
#endif

	return false;
}

const struct net_eth_addr *net_eth_broadcast_addr(void);

/** @endcond */

/**
 * @brief Convert IPv4 multicast address to Ethernet address.
 *
 * @param ipv4_addr IPv4 multicast address
 * @param mac_addr Output buffer for Ethernet address
 */
void net_eth_ipv4_mcast_to_mac_addr(const struct in_addr *ipv4_addr,
				    struct net_eth_addr *mac_addr);

/**
 * @brief Convert IPv6 multicast address to Ethernet address.
 *
 * @param ipv6_addr IPv6 multicast address
 * @param mac_addr Output buffer for Ethernet address
 */
void net_eth_ipv6_mcast_to_mac_addr(const struct in6_addr *ipv6_addr,
				    struct net_eth_addr *mac_addr);

/**
 * @brief Return ethernet device hardware capability information.
 *
 * @param iface Network interface
 *
 * @return Hardware capabilities
 */
static inline
enum ethernet_hw_caps net_eth_get_hw_capabilities(struct net_if *iface)
{
	const struct ethernet_api *eth =
		(struct ethernet_api *)net_if_get_device(iface)->api;

	if (!eth->get_capabilities) {
		return (enum ethernet_hw_caps)0;
	}

	return eth->get_capabilities(net_if_get_device(iface));
}

/**
 * @brief Add VLAN tag to the interface.
 *
 * @param iface Interface to use.
 * @param tag VLAN tag to add
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_VLAN)
int net_eth_vlan_enable(struct net_if *iface, uint16_t tag);
#else
static inline int net_eth_vlan_enable(struct net_if *iface, uint16_t tag)
{
	return -EINVAL;
}
#endif

/**
 * @brief Remove VLAN tag from the interface.
 *
 * @param iface Interface to use.
 * @param tag VLAN tag to remove
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_VLAN)
int net_eth_vlan_disable(struct net_if *iface, uint16_t tag);
#else
static inline int net_eth_vlan_disable(struct net_if *iface, uint16_t tag)
{
	return -EINVAL;
}
#endif

/**
 * @brief Return VLAN tag specified to network interface
 *
 * @param iface Network interface.
 *
 * @return VLAN tag for this interface or NET_VLAN_TAG_UNSPEC if VLAN
 * is not configured for that interface.
 */
#if defined(CONFIG_NET_VLAN)
uint16_t net_eth_get_vlan_tag(struct net_if *iface);
#else
static inline uint16_t net_eth_get_vlan_tag(struct net_if *iface)
{
	return NET_VLAN_TAG_UNSPEC;
}
#endif

/**
 * @brief Return network interface related to this VLAN tag
 *
 * @param iface Master network interface. This is used to get the
 *        pointer to ethernet L2 context
 * @param tag VLAN tag
 *
 * @return Network interface related to this tag or NULL if no such interface
 * exists.
 */
#if defined(CONFIG_NET_VLAN)
struct net_if *net_eth_get_vlan_iface(struct net_if *iface, uint16_t tag);
#else
static inline
struct net_if *net_eth_get_vlan_iface(struct net_if *iface, uint16_t tag)
{
	return NULL;
}
#endif

/**
 * @brief Check if VLAN is enabled for a specific network interface.
 *
 * @param ctx Ethernet context
 * @param iface Network interface
 *
 * @return True if VLAN is enabled for this network interface, false if not.
 */
#if defined(CONFIG_NET_VLAN)
bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
			     struct net_if *iface);
#else
static inline bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
					   struct net_if *iface)
{
	return false;
}
#endif

/**
 * @brief Get VLAN status for a given network interface (enabled or not).
 *
 * @param iface Network interface
 *
 * @return True if VLAN is enabled for this network interface, false if not.
 */
#if defined(CONFIG_NET_VLAN)
bool net_eth_get_vlan_status(struct net_if *iface);
#else
static inline bool net_eth_get_vlan_status(struct net_if *iface)
{
	return false;
}
#endif

#if defined(CONFIG_NET_VLAN)
#define Z_ETH_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			      config, prio, api, mtu)			\
	Z_DEVICE_STATE_DEFINE(dev_id);					\
	Z_DEVICE_DEFINE(node_id, dev_id, name, init_fn, pm, data,	\
			config, POST_KERNEL, prio, api,			\
			&Z_DEVICE_STATE_NAME(dev_id));			\
	NET_L2_DATA_INIT(dev_id, 0, NET_L2_GET_CTX_TYPE(ETHERNET_L2));	\
	NET_IF_INIT(dev_id, 0, ETHERNET_L2, mtu, NET_VLAN_MAX_COUNT)

#else /* CONFIG_NET_VLAN */

#define Z_ETH_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			      config, prio, api, mtu)			\
	Z_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			  config, prio, api, ETHERNET_L2,		\
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), mtu)
#endif /* CONFIG_NET_VLAN */

/**
 * @brief Create an Ethernet network interface and bind it to network device.
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
#define ETH_NET_DEVICE_INIT(dev_id, name, init_fn, pm, data, config,	\
			    prio, api, mtu)				\
	Z_ETH_NET_DEVICE_INIT(DT_INVALID_NODE, dev_id, name, init_fn,	\
			      pm, data, config, prio, api, mtu)

/**
 * @brief Like ETH_NET_DEVICE_INIT but taking metadata from a devicetree.
 * Create an Ethernet network interface and bind it to network device.
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
#define ETH_NET_DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config,	\
				 prio, api, mtu)			\
	Z_ETH_NET_DEVICE_INIT(node_id, Z_DEVICE_DT_DEV_ID(node_id),	\
			      DEVICE_DT_NAME(node_id), init_fn, pm,	\
			      data, config, prio, api, mtu)

/**
 * @brief Like ETH_NET_DEVICE_DT_DEFINE for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number.  This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to ETH_NET_DEVICE_DT_DEFINE.
 *
 * @param ... other parameters as expected by ETH_NET_DEVICE_DT_DEFINE.
 */
#define ETH_NET_DEVICE_DT_INST_DEFINE(inst, ...) \
	ETH_NET_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Inform ethernet L2 driver that ethernet carrier is detected.
 * This happens when cable is connected.
 *
 * @param iface Network interface
 */
void net_eth_carrier_on(struct net_if *iface);

/**
 * @brief Inform ethernet L2 driver that ethernet carrier was lost.
 * This happens when cable is disconnected.
 *
 * @param iface Network interface
 */
void net_eth_carrier_off(struct net_if *iface);

/**
 * @brief Set promiscuous mode either ON or OFF.
 *
 * @param iface Network interface
 *
 * @param enable on (true) or off (false)
 *
 * @return 0 if mode set or unset was successful, <0 otherwise.
 */
int net_eth_promisc_mode(struct net_if *iface, bool enable);

/**
 * @brief Set TX-Injection mode either ON or OFF.
 *
 * @param iface Network interface
 *
 * @param enable on (true) or off (false)
 *
 * @return 0 if mode set or unset was successful, <0 otherwise.
 */
int net_eth_txinjection_mode(struct net_if *iface, bool enable);

/**
 * @brief Set or unset HW filtering for MAC address @p mac.
 *
 * @param iface Network interface
 * @param mac Pointer to an ethernet MAC address
 * @param type Filter type, either source or destination
 * @param enable Set (true) or unset (false)
 *
 * @return 0 if filter set or unset was successful, <0 otherwise.
 */
int net_eth_mac_filter(struct net_if *iface, struct net_eth_addr *mac,
		       enum ethernet_filter_type type, bool enable);
/**
 * @brief Return PTP clock that is tied to this ethernet network interface.
 *
 * @param iface Network interface
 *
 * @return Pointer to PTP clock if found, NULL if not found or if this
 * ethernet interface does not support PTP.
 */
#if defined(CONFIG_PTP_CLOCK)
const struct device *net_eth_get_ptp_clock(struct net_if *iface);
#else
static inline const struct device *net_eth_get_ptp_clock(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NULL;
}
#endif

/**
 * @brief Return PTP clock that is tied to this ethernet network interface
 * index.
 *
 * @param index Network interface index
 *
 * @return Pointer to PTP clock if found, NULL if not found or if this
 * ethernet interface index does not support PTP.
 */
__syscall const struct device *net_eth_get_ptp_clock_by_index(int index);

/**
 * @brief Return PTP port number attached to this interface.
 *
 * @param iface Network interface
 *
 * @return Port number, no such port if < 0
 */
#if defined(CONFIG_NET_L2_PTP)
int net_eth_get_ptp_port(struct net_if *iface);
#else
static inline int net_eth_get_ptp_port(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENODEV;
}
#endif /* CONFIG_NET_L2_PTP */

/**
 * @brief Set PTP port number attached to this interface.
 *
 * @param iface Network interface
 * @param port Port number to set
 */
#if defined(CONFIG_NET_L2_PTP)
void net_eth_set_ptp_port(struct net_if *iface, int port);
#else
static inline void net_eth_set_ptp_port(struct net_if *iface, int port)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(port);
}
#endif /* CONFIG_NET_L2_PTP */

/**
 * @brief Check if the Ethernet L2 network interface can perform Wi-Fi.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface supports Wi-Fi, False otherwise.
 */
static inline bool net_eth_type_is_wifi(struct net_if *iface)
{
	const struct ethernet_context *ctx = (struct ethernet_context *)
		net_if_l2_data(iface);

	return ctx->eth_if_type == L2_ETH_IF_TYPE_WIFI;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/ethernet.h>

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_H_ */
