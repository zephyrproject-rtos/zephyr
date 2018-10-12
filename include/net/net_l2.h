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

#include <net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Layer 2 abstraction layer
 * @defgroup net_l2 Network L2 Abstraction Layer
 * @ingroup networking
 * @{
 */

struct net_if;

enum net_l2_flags {
	/** IP multicast supported */
	NET_L2_MULTICAST			= BIT(0),

	/** Do not joint solicited node multicast group */
	NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE	= BIT(1),

	/** Is promiscuous mode supported */
	NET_L2_PROMISC_MODE			= BIT(2),
} __packed;

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
	 */
	enum net_verdict (*send)(struct net_if *iface, struct net_pkt *pkt);

	/**
	 * This function is used to get the amount of bytes the net core should
	 * reserve as headroom in a net packet. Such space is relevant to L2
	 * layer only.
	 */
	u16_t (*reserve)(struct net_if *iface, void *data);

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

#define NET_L2_GET_NAME(_name) (__net_l2_##_name)
#define NET_L2_DECLARE_PUBLIC(_name)					\
	extern const struct net_l2 NET_L2_GET_NAME(_name)
#define NET_L2_GET_CTX_TYPE(_name) _name##_CTX_TYPE

#ifdef CONFIG_NET_L2_DUMMY
#define DUMMY_L2		DUMMY
#define DUMMY_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(DUMMY_L2);
#endif /* CONFIG_NET_L2_DUMMY */

#ifdef CONFIG_NET_L2_ETHERNET
#define ETHERNET_L2		ETHERNET
NET_L2_DECLARE_PUBLIC(ETHERNET_L2);
#endif /* CONFIG_NET_L2_ETHERNET */

#ifdef CONFIG_NET_L2_IEEE802154
#define IEEE802154_L2		IEEE802154
NET_L2_DECLARE_PUBLIC(IEEE802154_L2);
#endif /* CONFIG_NET_L2_IEEE802154 */

#ifdef CONFIG_NET_L2_BT
#define BLUETOOTH_L2		BLUETOOTH
#define BLUETOOTH_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(BLUETOOTH_L2);
#endif /* CONFIG_NET_L2_BT */

#ifdef CONFIG_NET_L2_OPENTHREAD
#define OPENTHREAD_L2		OPENTHREAD
NET_L2_DECLARE_PUBLIC(OPENTHREAD_L2);
#endif /* CONFIG_NET_L2_OPENTHREAD */

#define NET_L2_INIT(_name, _recv_fn, _send_fn, _reserve_fn, _enable_fn, \
		    _get_flags_fn)					\
	const struct net_l2 (NET_L2_GET_NAME(_name)) __used		\
	__attribute__((__section__(".net_l2.init"))) = {		\
		.recv = (_recv_fn),					\
		.send = (_send_fn),					\
		.reserve = (_reserve_fn),				\
		.enable = (_enable_fn),					\
		.get_flags = (_get_flags_fn),				\
	}

#define NET_L2_GET_DATA(name, sfx) (__net_l2_data_##name##sfx)

#define NET_L2_DATA_INIT(name, sfx, ctx_type)				\
	static ctx_type NET_L2_GET_DATA(name, sfx) __used		\
	__attribute__((__section__(".net_l2.data")));

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_L2_H_ */
