/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for network L2 interface
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_L2_H_
#define ZEPHYR_INCLUDE_NET_NET_L2_H_

#include <zephyr/device.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/capture.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Layer 2 abstraction layer
 * @defgroup net_l2 Network L2 Abstraction Layer
 * @since 1.5
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

struct net_if;

/** L2 flags */
enum net_l2_flags {
	/** IP multicast supported */
	NET_L2_MULTICAST			= BIT(0),

	/** Do not join solicited node multicast group */
	NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE	= BIT(1),

	/** Is promiscuous mode supported */
	NET_L2_PROMISC_MODE			= BIT(2),

	/** Is this L2 point-to-point with tunneling so no need to have
	 * IP address etc to network interface.
	 */
	NET_L2_POINT_TO_POINT			= BIT(3),
} __packed;

/**
 * @brief Network L2 structure
 *
 * Used to provide an interface to lower network stack.
 */
struct net_l2 {
	/**
	 * This function is used by net core to get iface's L2 layer parsing
	 * what's relevant to itself.
	 */
	enum net_verdict (*recv)(struct net_if *iface, struct net_pkt *pkt);

	/**
	 * This function is used by net core to push a packet to lower layer
	 * (interface's L2), which in turn might work on the packet relevantly.
	 * (adding proper header etc...)
	 * Returns a negative error code, or the number of bytes sent otherwise.
	 */
	int (*send)(struct net_if *iface, struct net_pkt *pkt);

	/**
	 * This function is used to enable/disable traffic over a network
	 * interface. The function returns <0 if error and >=0 if no error.
	 */
	int (*enable)(struct net_if *iface, bool state);

	/**
	 * Return L2 flags for the network interface.
	 */
	enum net_l2_flags (*get_flags)(struct net_if *iface);
};

/** @cond INTERNAL_HIDDEN */
#define NET_L2_GET_NAME(_name) _net_l2_##_name
#define NET_L2_DECLARE_PUBLIC(_name)					\
	extern const struct net_l2 NET_L2_GET_NAME(_name)
#define NET_L2_GET_CTX_TYPE(_name) _name##_CTX_TYPE

#define VIRTUAL_L2		VIRTUAL
NET_L2_DECLARE_PUBLIC(VIRTUAL_L2);

#define DUMMY_L2		DUMMY
#define DUMMY_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(DUMMY_L2);

#define OFFLOADED_NETDEV_L2 OFFLOADED_NETDEV
NET_L2_DECLARE_PUBLIC(OFFLOADED_NETDEV_L2);

#define ETHERNET_L2		ETHERNET
NET_L2_DECLARE_PUBLIC(ETHERNET_L2);

#define PPP_L2			PPP
NET_L2_DECLARE_PUBLIC(PPP_L2);

#define IEEE802154_L2		IEEE802154
NET_L2_DECLARE_PUBLIC(IEEE802154_L2);

#define OPENTHREAD_L2		OPENTHREAD
NET_L2_DECLARE_PUBLIC(OPENTHREAD_L2);

#define CANBUS_RAW_L2		CANBUS_RAW
#define CANBUS_RAW_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(CANBUS_RAW_L2);

#ifdef CONFIG_NET_L2_CUSTOM_IEEE802154
#ifndef CUSTOM_IEEE802154_L2
#define CUSTOM_IEEE802154_L2	CUSTOM_IEEE802154
#endif
#define CUSTOM_IEEE802154_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(CUSTOM_IEEE802154_L2);
#endif /* CONFIG_NET_L2_CUSTOM_IEEE802154 */

#define NET_L2_INIT(_name, _recv_fn, _send_fn, _enable_fn, _get_flags_fn) \
	const STRUCT_SECTION_ITERABLE(net_l2,				\
				      NET_L2_GET_NAME(_name)) = {	\
		.recv = (_recv_fn),					\
		.send = (_send_fn),					\
		.enable = (_enable_fn),					\
		.get_flags = (_get_flags_fn),				\
	}

#define NET_L2_GET_DATA(name, sfx) _net_l2_data_##name##sfx

#define NET_L2_DATA_INIT(name, sfx, ctx_type)				\
	static ctx_type NET_L2_GET_DATA(name, sfx) __used;

typedef int (*net_l2_send_t)(const struct device *dev, struct net_pkt *pkt);

static inline int net_l2_send(net_l2_send_t send_fn,
			      const struct device *dev,
			      struct net_if *iface,
			      struct net_pkt *pkt)
{
	net_capture_pkt(iface, pkt);

	return send_fn(dev, pkt);
}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_L2_H_ */
