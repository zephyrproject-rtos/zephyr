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

#include <zephyr/types.h>
#include <stdbool.h>
#include <atomic.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/lldp.h>
#include <misc/util.h>
#include <net/net_if.h>
#include <net/ethernet_vlan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ethernet support functions
 * @defgroup ethernet Ethernet Support Functions
 * @ingroup networking
 * @{
 */

struct net_eth_addr {
	u8_t addr[6];
};

#define NET_ETH_HDR(pkt) ((struct net_eth_hdr *)net_pkt_ll(pkt))

#define NET_ETH_PTYPE_ARP		0x0806
#define NET_ETH_PTYPE_IP		0x0800
#define NET_ETH_PTYPE_IPV6		0x86dd
#define NET_ETH_PTYPE_VLAN		0x8100
#define NET_ETH_PTYPE_PTP		0x88f7
#define NET_ETH_PTYPE_LLDP		0x88cc

#define NET_ETH_MINIMAL_FRAME_SIZE	60

enum ethernet_hw_caps {
	/** TX Checksum offloading supported */
	ETHERNET_HW_TX_CHKSUM_OFFLOAD	= BIT(0),

	/** RX Checksum offloading supported */
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
};

enum ethernet_config_type {
	ETHERNET_CONFIG_TYPE_AUTO_NEG,
	ETHERNET_CONFIG_TYPE_LINK,
	ETHERNET_CONFIG_TYPE_DUPLEX,
	ETHERNET_CONFIG_TYPE_MAC_ADDRESS,
	ETHERNET_CONFIG_TYPE_QAV_PARAM,
	ETHERNET_CONFIG_TYPE_PROMISC_MODE,
	ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM,
	ETHERNET_CONFIG_TYPE_FILTER,
};

enum ethernet_qav_param_type {
	ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH,
	ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE,
	ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE,
	ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS,
	ETHERNET_QAV_PARAM_TYPE_STATUS,
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

enum ethernet_filter_type {
	ETHERNET_FILTER_TYPE_SRC_MAC_ADDRESS,
	ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS,
};

struct ethernet_filter {
	/** Type of filter */
	enum ethernet_filter_type type;
	/** MAC address to filter */
	struct net_eth_addr mac_address;
	/** Set (true) or unset (false) the filter */
	bool set;
};

struct ethernet_config {
/** @cond ignore */
	union {
		bool auto_negotiation;
		bool full_duplex;
		bool promisc_mode;

		struct {
			bool link_10bt;
			bool link_100bt;
			bool link_1000bt;
		} l;

		struct net_eth_addr mac_address;

		struct ethernet_qav_param qav_param;

		int priority_queues_num;

		struct ethernet_filter filter;
	};
/* @endcond */
};

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
	struct net_stats_eth *(*get_stats)(struct device *dev);
#endif

	/** Start the device */
	int (*start)(struct device *dev);

	/** Stop the device */
	int (*stop)(struct device *dev);

	/** Get the device capabilities */
	enum ethernet_hw_caps (*get_capabilities)(struct device *dev);

	/** Set specific hardware configuration */
	int (*set_config)(struct device *dev,
			  enum ethernet_config_type type,
			  const struct ethernet_config *config);

	/** Get hardware specific configuration */
	int (*get_config)(struct device *dev,
			  enum ethernet_config_type type,
			  struct ethernet_config *config);

#if defined(CONFIG_NET_VLAN)
	/** The IP stack will call this function when a VLAN tag is enabled
	 * or disabled. If enable is set to true, then the VLAN tag was added,
	 * if it is false then the tag was removed. The driver can utilize
	 * this information if needed.
	 */
	int (*vlan_setup)(struct device *dev, struct net_if *iface,
			  u16_t tag, bool enable);
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK)
	/** Return ptp_clock device that is tied to this ethernet device */
	struct device *(*get_ptp_clock)(struct device *dev);
#endif /* CONFIG_PTP_CLOCK */
};

struct net_eth_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	u16_t type;
} __packed;

struct ethernet_vlan {
	/** Network interface that has VLAN enabled */
	struct net_if *iface;

	/** VLAN tag */
	u16_t tag;
};

#if defined(CONFIG_NET_VLAN_COUNT)
#define NET_VLAN_MAX_COUNT CONFIG_NET_VLAN_COUNT
#else
/* Even thou there are no VLAN support, the minimum count must be set to 1.
 */
#define NET_VLAN_MAX_COUNT 1
#endif

#if defined(CONFIG_NET_LLDP)
struct ethernet_lldp {
	/** Used for track timers */
	sys_snode_t node;

	/** LLDP information element related to this network interface. */
	const struct net_lldpdu *lldpdu;

	/** Network interface that has LLDP supported. */
	struct net_if *iface;

	/** LLDP TX timer start time */
	s64_t tx_timer_start;

	/** LLDP TX timeout */
	u32_t tx_timer_timeout;

	/** LLDP RX callback function */
	net_lldp_recv_cb_t cb;
};
#endif /* CONFIG_NET_LLDP */

/** Ethernet L2 context that is needed for VLAN */
struct ethernet_context {
#if defined(CONFIG_NET_VLAN)
	struct ethernet_vlan vlan[NET_VLAN_MAX_COUNT];

	/** Array that will help when checking if VLAN is enabled for
	 * some specific network interface. Requires that VLAN count
	 * NET_VLAN_MAX_COUNT is not smaller than the actual number
	 * of network interfaces.
	 */
	ATOMIC_DEFINE(interfaces, NET_VLAN_MAX_COUNT);
#endif

	struct {
		/** Carrier ON/OFF handler worker. This is used to create
		 * network interface UP/DOWN event when ethernet L2 driver
		 * notices carrier ON/OFF situation. We must not create another
		 * network management event from inside management handler thus
		 * we use worker thread to trigger the UP/DOWN event.
		 */
		struct k_work work;

		/** Network interface that is detecting carrier ON/OFF event.
		 */
		struct net_if *iface;
	} carrier_mgmt;

#if defined(CONFIG_NET_LLDP)
	struct ethernet_lldp lldp[NET_VLAN_MAX_COUNT];
#endif

	/**
	 * This tells what L2 features does ethernet support.
	 */
	enum net_l2_flags ethernet_l2_flags;

#if defined(CONFIG_NET_GPTP)
	/** The gPTP port number for this network device. We need to store the
	 * port number here so that we do not need to fetch it for every
	 * incoming gPTP packet.
	 */
	int port;
#endif

#if defined(CONFIG_NET_VLAN)
	/** Flag that tells whether how many VLAN tags are enabled for this
	 * context. The same information can be dug from the vlan array but
	 * this saves some time in RX path.
	 */
	s8_t vlan_enabled;
#endif

	/** Is this context already initialized */
	bool is_init;
};

#define ETHERNET_L2_CTX_TYPE	struct ethernet_context

/**
 * @brief Initialize Ethernet L2 stack for a given interface
 *
 * @param iface A valid pointer to a network interface
 */
void ethernet_init(struct net_if *iface);

#if defined(CONFIG_NET_VLAN)
/* Separate header for VLAN as some of device interfaces might not
 * support VLAN.
 */
struct net_eth_vlan_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	struct {
		u16_t tpid; /* tag protocol id  */
		u16_t tci;  /* tag control info */
	} vlan;
	u16_t type;
} __packed;

#endif /* CONFIG_NET_VLAN */

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

const struct net_eth_addr *net_eth_broadcast_addr(void);

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
		net_if_get_device(iface)->driver_api;

	if (!eth->get_capabilities) {
		return 0;
	}

	return eth->get_capabilities(net_if_get_device(iface));
}

#if defined(CONFIG_NET_VLAN)
/**
 * @brief Add VLAN tag to the interface.
 *
 * @param iface Interface to use.
 * @param tag VLAN tag to add
 *
 * @return 0 if ok, <0 if error
 */
int net_eth_vlan_enable(struct net_if *iface, u16_t tag);

/**
 * @brief Remove VLAN tag from the interface.
 *
 * @param iface Interface to use.
 * @param tag VLAN tag to remove
 *
 * @return 0 if ok, <0 if error
 */
int net_eth_vlan_disable(struct net_if *iface, u16_t tag);

/**
 * @brief Return VLAN tag specified to network interface
 *
 * @param iface Network interface.
 *
 * @return VLAN tag for this interface or NET_VLAN_TAG_UNSPEC if VLAN
 * is not configured for that interface.
 */
u16_t net_eth_get_vlan_tag(struct net_if *iface);

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
struct net_if *net_eth_get_vlan_iface(struct net_if *iface, u16_t tag);

/**
 * @brief Check if VLAN is enabled for a specific network interface.
 *
 * @param ctx Ethernet context
 * @param iface Network interface
 *
 * @return True if VLAN is enabled for this network interface, false if not.
 */
bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
			     struct net_if *iface);

/**
 * @brief Get VLAN status for a given network interface (enabled or not).
 *
 * @param iface Network interface
 *
 * @return True if VLAN is enabled for this network interface, false if not.
 */
bool net_eth_get_vlan_status(struct net_if *iface);

#define ETH_NET_DEVICE_INIT(dev_name, drv_name, init_fn,		 \
			    data, cfg_info, prio, api, mtu)		 \
	DEVICE_AND_API_INIT(dev_name, drv_name, init_fn, data,		 \
			    cfg_info, POST_KERNEL, prio, api);		 \
	NET_L2_DATA_INIT(dev_name, 0, NET_L2_GET_CTX_TYPE(ETHERNET_L2)); \
	NET_IF_INIT(dev_name, 0, ETHERNET_L2, mtu, NET_VLAN_MAX_COUNT)

#else /* CONFIG_NET_VLAN */

#define ETH_NET_DEVICE_INIT(dev_name, drv_name, init_fn,		\
			    data, cfg_info, prio, api, mtu)		\
	NET_DEVICE_INIT(dev_name, drv_name, init_fn,			\
			data, cfg_info, prio, api, ETHERNET_L2,		\
			NET_L2_GET_CTX_TYPE(ETHERNET_L2), mtu)

static inline int net_eth_vlan_enable(struct net_if *iface, u16_t vlan_tag)
{
	return -EINVAL;
}

static inline int net_eth_vlan_disable(struct net_if *iface, u16_t vlan_tag)
{
	return -EINVAL;
}

static inline u16_t net_eth_get_vlan_tag(struct net_if *iface)
{
	return NET_VLAN_TAG_UNSPEC;
}

static inline
struct net_if *net_eth_get_vlan_iface(struct net_if *iface, u16_t tag)
{
	return NULL;
}

static inline bool net_eth_get_vlan_status(struct net_if *iface)
{
	return false;
}
#endif /* CONFIG_NET_VLAN */

/**
 * @brief Fill ethernet header in network packet.
 *
 * @param ctx Ethernet context
 * @param pkt Network packet
 * @param ptype Upper level protocol type (in network byte order)
 * @param src Source ethernet address
 * @param dst Destination ethernet address
 *
 * @return Pointer to ethernet header struct inside net_buf.
 */
struct net_eth_hdr *net_eth_fill_header(struct ethernet_context *ctx,
					struct net_pkt *pkt,
					u32_t ptype,
					u8_t *src,
					u8_t *dst);

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
 * @brief Return PTP clock that is tied to this ethernet network interface.
 *
 * @param iface Network interface
 *
 * @return Pointer to PTP clock if found, NULL if not found or if this
 * ethernet interface does not support PTP.
 */
struct device *net_eth_get_ptp_clock(struct net_if *iface);

#if defined(CONFIG_NET_GPTP)
/**
 * @brief Return gPTP port number attached to this interface.
 *
 * @param iface Network interface
 *
 * @return Port number, no such port if < 0
 */
int net_eth_get_ptp_port(struct net_if *iface);

/**
 * @brief Set gPTP port number attached to this interface.
 *
 * @param iface Network interface
 * @param port Port number to set
 */
void net_eth_set_ptp_port(struct net_if *iface, int port);
#else
static inline int net_eth_get_ptp_port(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENODEV;
}
#endif /* CONFIG_NET_GPTP */

struct net_lldpdu;

/**
 * @brief Set LLDP protocol data unit (LLDPDU) for the network interface.
 *
 * @param iface Network interface
 * @param lldpdu LLDPDU pointer
 *
 * @return <0 if error, index in lldp array if iface is found there
 */
#if defined(CONFIG_NET_LLDP)
int net_eth_set_lldpdu(struct net_if *iface, const struct net_lldpdu *lldpdu);
#else
static inline int net_eth_set_lldpdu(struct net_if *iface,
				     const struct net_lldpdu *lldpdu)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(lldpdu);

	return -ENOTSUP;
}
#endif

/**
 * @brief Unset LLDP protocol data unit (LLDPDU) for the network interface.
 *
 * @param iface Network interface
 */
#if defined(CONFIG_NET_LLDP)
void net_eth_unset_lldpdu(struct net_if *iface);
#else
static inline void net_eth_unset_lldpdu(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_H_ */
