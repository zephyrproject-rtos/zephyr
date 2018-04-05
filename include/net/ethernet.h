/** @file
 @brief Ethernet

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ETHERNET_H
#define __ETHERNET_H

#include <zephyr/types.h>
#include <stdbool.h>
#include <atomic.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
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

#define NET_ETH_HDR(pkt) ((struct net_eth_hdr *)net_pkt_ll(pkt))

#define NET_ETH_PTYPE_ARP		0x0806
#define NET_ETH_PTYPE_IP		0x0800
#define NET_ETH_PTYPE_IPV6		0x86dd
#define NET_ETH_PTYPE_VLAN		0x8100

#define NET_ETH_MINIMAL_FRAME_SIZE	60

enum eth_hw_caps {
	/** TX Checksum offloading supported */
	ETH_HW_TX_CHKSUM_OFFLOAD	= BIT(0),

	/** RX Checksum offloading supported */
	ETH_HW_RX_CHKSUM_OFFLOAD	= BIT(1),

	/** VLAN supported */
	ETH_HW_VLAN			= BIT(2),

	/** Enabling/disabling auto negotiation supported */
	ETH_AUTO_NEGOTIATION_SET	= BIT(3),

	/** 10 Mbits link supported */
	ETH_LINK_10BASE_T		= BIT(4),

	/** 100 Mbits link supported */
	ETH_LINK_100BASE_T		= BIT(5),

	/** 1 Gbits link supported */
	ETH_LINK_1000BASE_T		= BIT(6),

	/** Changing duplex (half/full) supported */
	ETH_DUPLEX_SET			= BIT(7),
};

struct ethernet_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Get the device capabilities */
	enum eth_hw_caps (*get_capabilities)(struct device *dev);

#if defined(CONFIG_NET_VLAN)
	/** The IP stack will call this function when a VLAN tag is enabled
	 * or disabled. If enable is set to true, then the VLAN tag was added,
	 * if it is false then the tag was removed. The driver can utilize
	 * this information if needed.
	 */
	int (*vlan_setup)(struct device *dev, struct net_if *iface,
			  u16_t tag, bool enable);
#endif /* CONFIG_NET_VLAN */
};

struct net_eth_addr {
	u8_t addr[6];
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

	/** Flag that tells whether how many VLAN tags are enabled for this
	 * context. The same information can be dug from the vlan array but
	 * this saves some time in RX path.
	 */
	s8_t vlan_enabled;

	/** Is this context already initialized */
	bool is_init;
#endif
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
enum eth_hw_caps net_eth_get_hw_capabilities(struct net_if *iface)
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
#endif /* CONFIG_NET_VLAN */

/**
 * @brief Fill ethernet header in network packet.
 *
 * @param ctx Ethernet context
 * @param pkt Network packet
 * @param frag Ethernet header in packet
 * @param ptype Upper level protocol type (in network byte order)
 * @param src Source ethernet address
 * @param dst Destination ethernet address
 *
 * @return Pointer to ethernet header struct inside net_buf.
 */
struct net_eth_hdr *net_eth_fill_header(struct ethernet_context *ctx,
					struct net_pkt *pkt,
					struct net_buf *frag,
					u32_t ptype,
					u8_t *src,
					u8_t *dst);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __ETHERNET_H */
