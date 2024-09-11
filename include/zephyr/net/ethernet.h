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
#include <zephyr/net/lldp.h>
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
 * @since 1.0
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#define NET_ETH_ADDR_LEN		6U /**< Ethernet MAC address length */

/** Ethernet address */
struct net_eth_addr {
	uint8_t addr[NET_ETH_ADDR_LEN]; /**< Buffer storing the address */
};

/** @cond INTERNAL_HIDDEN */

#define NET_ETH_HDR(pkt) ((struct net_eth_hdr *)net_pkt_data(pkt))

#define NET_ETH_PTYPE_CAN		0x000C /* CAN: Controller Area Network */
#define NET_ETH_PTYPE_CANFD		0x000D /* CANFD: CAN flexible data rate*/
#define NET_ETH_PTYPE_HDLC		0x0019 /* HDLC frames (like in PPP) */
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
#if !defined(ETH_P_EAPOL)
#define ETH_P_EAPOL	NET_ETH_PTYPE_EAPOL
#endif
#if !defined(ETH_P_IEEE802154)
#define  ETH_P_IEEE802154 NET_ETH_PTYPE_IEEE802154
#endif
#if !defined(ETH_P_CAN)
#define ETH_P_CAN	NET_ETH_PTYPE_CAN
#endif
#if !defined(ETH_P_CANFD)
#define ETH_P_CANFD	NET_ETH_PTYPE_CANFD
#endif
#if !defined(ETH_P_HDLC)
#define ETH_P_HDLC	NET_ETH_PTYPE_HDLC
#endif

/** @endcond */

#define NET_ETH_MINIMAL_FRAME_SIZE	60   /**< Minimum Ethernet frame size */
#define NET_ETH_MTU			1500 /**< Ethernet MTU size */

/** @cond INTERNAL_HIDDEN */

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

/** @brief Ethernet hardware capabilities */
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

	/** DSA switch slave port */
	ETHERNET_DSA_SLAVE_PORT		= BIT(15),

	/** DSA switch master port */
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
	ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT,
	ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT
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

/** Ethernet T1S specific parameters */
struct ethernet_t1s_param {
	/** Type of T1S parameter */
	enum ethernet_t1s_param_type type;
	union {
		/**
		 * PLCA is the Physical Layer (PHY) Collision
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

/** Ethernet Qav specific parameters */
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

/** Ethernet Qbv specific parameters */
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

		/** Gate control information */
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

		/**
		 * The time values are set in one go when type is set to
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

/** Ethernet Qbu specific parameters */
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

		/** sequence of framePreemptionAdminStatus values */
		enum ethernet_qbu_preempt_status
				frame_preempt_statuses[NET_TC_TX_COUNT];

		/** True if Qbu is enabled or not */
		bool enabled;

		/** Link partner status (from Qbr) */
		bool link_partner_status;

		/**
		 * Additional fragment size (from Qbr). The minimum non-final
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

/** Ethernet filter description */
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

/** Ethernet TXTIME specific parameters */
struct ethernet_txtime_param {
	/** Type of TXTIME parameter */
	enum ethernet_txtime_param_type type;
	/** Queue number for configuring TXTIME */
	int queue_id;
	/** Enable or disable TXTIME per queue */
	bool enable_txtime;
};

/** Protocols that are supported by checksum offloading */
enum ethernet_checksum_support {
	/** Device does not support any L3/L4 checksum offloading */
	ETHERNET_CHECKSUM_SUPPORT_NONE			= NET_IF_CHECKSUM_NONE_BIT,
	/** Device supports checksum offloading for the IPv4 header */
	ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER		= NET_IF_CHECKSUM_IPV4_HEADER_BIT,
	/** Device supports checksum offloading for ICMPv4 payload (implies IPv4 header) */
	ETHERNET_CHECKSUM_SUPPORT_IPV4_ICMP		= NET_IF_CHECKSUM_IPV4_ICMP_BIT,
	/** Device supports checksum offloading for the IPv6 header */
	ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER		= NET_IF_CHECKSUM_IPV6_HEADER_BIT,
	/** Device supports checksum offloading for ICMPv6 payload (implies IPv6 header) */
	ETHERNET_CHECKSUM_SUPPORT_IPV6_ICMP		= NET_IF_CHECKSUM_IPV6_ICMP_BIT,
	/** Device supports TCP checksum offloading for all supported IP protocols */
	ETHERNET_CHECKSUM_SUPPORT_TCP			= NET_IF_CHECKSUM_TCP_BIT,
	/** Device supports UDP checksum offloading for all supported IP protocols */
	ETHERNET_CHECKSUM_SUPPORT_UDP			= NET_IF_CHECKSUM_UDP_BIT,
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

		enum ethernet_checksum_support chksum_support;

		struct ethernet_filter filter;
	};
};

/** @endcond */

/** Ethernet L2 API operations. */
struct ethernet_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Collect optional ethernet specific statistics. This pointer
	 * should be set by driver if statistics needs to be collected
	 * for that driver.
	 */
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
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

	/** The IP stack will call this function when a VLAN tag is enabled
	 * or disabled. If enable is set to true, then the VLAN tag was added,
	 * if it is false then the tag was removed. The driver can utilize
	 * this information if needed.
	 */
#if defined(CONFIG_NET_VLAN)
	int (*vlan_setup)(const struct device *dev, struct net_if *iface,
			  uint16_t tag, bool enable);
#endif /* CONFIG_NET_VLAN */

	/** Return ptp_clock device that is tied to this ethernet device */
#if defined(CONFIG_PTP_CLOCK)
	const struct device *(*get_ptp_clock)(const struct device *dev);
#endif /* CONFIG_PTP_CLOCK */

	/** Return PHY device that is tied to this ethernet device */
	const struct device *(*get_phy)(const struct device *dev);

	/** Send a network packet */
	int (*send)(const struct device *dev, struct net_pkt *pkt);
};

/** @cond INTERNAL_HIDDEN */

/* Make sure that the network interface API is properly setup inside
 * Ethernet API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct ethernet_api, iface_api) == 0);

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

/** Ethernet LLDP specific parameters */
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

/** @cond INTERNAL_HIDDEN */

enum ethernet_flags {
	ETH_CARRIER_UP,
};

/** Ethernet L2 context that is needed for VLAN */
struct ethernet_context {
	/** Flags representing ethernet state, which are accessed from multiple
	 * threads.
	 */
	atomic_t flags;

#if defined(CONFIG_NET_ETHERNET_BRIDGE)
	struct net_if *bridge;
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

/** @endcond */

/**
 * @brief Check if the Ethernet MAC address is a broadcast address.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is a broadcast address, false if not
 */
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

/**
 * @brief Check if the Ethernet MAC address is a all zeroes address.
 *
 * @param addr A valid pointer to an Ethernet MAC address.
 *
 * @return true if address is an all zeroes address, false if not
 */
static inline bool net_eth_is_addr_all_zeroes(struct net_eth_addr *addr)
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

/**
 * @brief Check if the Ethernet MAC address is unspecified.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is unspecified, false if not
 */
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

/**
 * @brief Check if the Ethernet MAC address is a multicast address.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is a multicast address, false if not
 */
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

/**
 * @brief Check if the Ethernet MAC address is a group address.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is a group address, false if not
 */
static inline bool net_eth_is_addr_group(struct net_eth_addr *addr)
{
	return addr->addr[0] & 0x01;
}

/**
 * @brief Check if the Ethernet MAC address is valid.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is valid, false if not
 */
static inline bool net_eth_is_addr_valid(struct net_eth_addr *addr)
{
	return !net_eth_is_addr_unspecified(addr) && !net_eth_is_addr_group(addr);
}

/**
 * @brief Check if the Ethernet MAC address is a LLDP multicast address.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is a LLDP multicast address, false if not
 */
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
#else
	ARG_UNUSED(addr);
#endif

	return false;
}

/**
 * @brief Check if the Ethernet MAC address is a PTP multicast address.
 *
 * @param addr A valid pointer to a Ethernet MAC address.
 *
 * @return true if address is a PTP multicast address, false if not
 */
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
#else
	ARG_UNUSED(addr);
#endif

	return false;
}

/**
 * @brief Return Ethernet broadcast address.
 *
 * @return Ethernet broadcast address.
 */
const struct net_eth_addr *net_eth_broadcast_addr(void);

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
	const struct device *dev = net_if_get_device(iface);
	const struct ethernet_api *api = (struct ethernet_api *)dev->api;

	if (!api || !api->get_capabilities) {
		return (enum ethernet_hw_caps)0;
	}

	return api->get_capabilities(dev);
}

/**
 * @brief Return ethernet device hardware configuration information.
 *
 * @param iface Network interface
 * @param type configuration type
 * @param config Ethernet configuration
 *
 * @return 0 if ok, <0 if error
 */
static inline
int net_eth_get_hw_config(struct net_if *iface, enum ethernet_config_type type,
			 struct ethernet_config *config)
{
	const struct ethernet_api *eth =
		(struct ethernet_api *)net_if_get_device(iface)->api;

	if (!eth->get_config) {
		return -ENOTSUP;
	}

	return eth->get_config(net_if_get_device(iface), type, config);
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
	ARG_UNUSED(iface);
	ARG_UNUSED(tag);

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
	ARG_UNUSED(iface);
	ARG_UNUSED(tag);

	return -EINVAL;
}
#endif

/**
 * @brief Return VLAN tag specified to network interface.
 *
 * Note that the interface parameter must be the VLAN interface,
 * and not the Ethernet one.
 *
 * @param iface VLAN network interface.
 *
 * @return VLAN tag for this interface or NET_VLAN_TAG_UNSPEC if VLAN
 * is not configured for that interface.
 */
#if defined(CONFIG_NET_VLAN)
uint16_t net_eth_get_vlan_tag(struct net_if *iface);
#else
static inline uint16_t net_eth_get_vlan_tag(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NET_VLAN_TAG_UNSPEC;
}
#endif

/**
 * @brief Return network interface related to this VLAN tag
 *
 * @param iface Main network interface (not the VLAN one).
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
	ARG_UNUSED(iface);
	ARG_UNUSED(tag);

	return NULL;
}
#endif

/**
 * @brief Return main network interface that is attached to this VLAN tag.
 *
 * @param iface VLAN network interface. This is used to get the
 *        pointer to ethernet L2 context
 *
 * @return Network interface related to this tag or NULL if no such interface
 * exists.
 */
#if defined(CONFIG_NET_VLAN)
struct net_if *net_eth_get_vlan_main(struct net_if *iface);
#else
static inline
struct net_if *net_eth_get_vlan_main(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NULL;
}
#endif

/**
 * @brief Check if there are any VLAN interfaces enabled to this specific
 *        Ethernet network interface.
 *
 * Note that the iface must be the actual Ethernet interface and not the
 * virtual VLAN interface.
 *
 * @param ctx Ethernet context
 * @param iface Ethernet network interface
 *
 * @return True if there are enabled VLANs for this network interface,
 *         false if not.
 */
#if defined(CONFIG_NET_VLAN)
bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
			     struct net_if *iface);
#else
static inline bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
					   struct net_if *iface)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(iface);

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
	ARG_UNUSED(iface);

	return false;
}
#endif

/**
 * @brief Check if the given interface is a VLAN interface.
 *
 * @param iface Network interface
 *
 * @return True if this network interface is VLAN one, false if not.
 */
#if defined(CONFIG_NET_VLAN)
bool net_eth_is_vlan_interface(struct net_if *iface);
#else
static inline bool net_eth_is_vlan_interface(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return false;
}
#endif

/** @cond INTERNAL_HIDDEN */

#if !defined(CONFIG_ETH_DRIVER_RAW_MODE)

#define Z_ETH_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, instance,	\
				       init_fn, pm, data, config, prio,	\
				       api, mtu)			\
	Z_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, instance,	\
				   init_fn, pm, data, config, prio,	\
				   api, ETHERNET_L2,			\
				   NET_L2_GET_CTX_TYPE(ETHERNET_L2), mtu)

#else /* CONFIG_ETH_DRIVER_RAW_MODE */

#define Z_ETH_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, instance,	\
				       init_fn, pm, data, config, prio,	\
				       api, mtu)			\
	Z_DEVICE_STATE_DEFINE(dev_id);					\
	Z_DEVICE_DEFINE(node_id, dev_id, name, init_fn, pm, data,	\
			config, POST_KERNEL, prio, api,			\
			&Z_DEVICE_STATE_NAME(dev_id));

#endif /* CONFIG_ETH_DRIVER_RAW_MODE */

#define Z_ETH_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			      config, prio, api, mtu)			\
	Z_ETH_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, 0,	\
				       init_fn, pm, data, config, prio,	\
				       api, mtu)

/** @endcond */

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
 * @brief Create multiple Ethernet network interfaces and bind them to network
 * devices.
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
 * @param mtu Maximum transfer unit in bytes for this network interface.
 */
#define ETH_NET_DEVICE_INIT_INSTANCE(dev_id, name, instance, init_fn,	\
				     pm, data, config, prio, api, mtu)	\
	Z_ETH_NET_DEVICE_INIT_INSTANCE(DT_INVALID_NODE, dev_id, name,	\
				       instance, init_fn, pm, data,	\
				       config, prio, api, mtu)

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
 * @brief Return the PHY device that is tied to this ethernet network interface.
 *
 * @param iface Network interface
 *
 * @return Pointer to PHY device if found, NULL if not found.
 */
const struct device *net_eth_get_phy(struct net_if *iface);

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

#include <zephyr/syscalls/ethernet.h>

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_H_ */
