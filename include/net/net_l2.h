/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for network L2 interface
 */

#ifndef __NET_L2_H__
#define __NET_L2_H__

#include <net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_if;

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
struct net_l2_offload_ip;
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

struct net_l2 {
	/**
	 * This function is used by net core to get iface's L2 layer parsing
	 * what's relevant to itself.
	 */
	enum net_verdict (*recv)(struct net_if *iface, struct net_buf *buf);

	/**
	 * This function is used by net core to push a buffer to lower layer
	 * (interface's L2), which in turn might work on the buffer relevantly.
	 * (adding proper header etc...)
	 */
	enum net_verdict (*send)(struct net_if *iface, struct net_buf *buf);

	/**
	 * This function is used to get the amount of bytes the net core should
	 * reserve as headroom in a net buffer. Such space is relevant to L2
	 * layer only.
	 */
	uint16_t (*reserve)(struct net_if *iface, void *data);

	/**
	 * This function is used to enable/disable traffic over a network
	 * interface.
	 */
	int (*enable)(struct net_if *iface, bool state);

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	struct net_l2_offload_ip *offload_ip;
#endif /* CONFIG_NET_L2_OFFLOAD_IP */
};

#define NET_L2_GET_NAME(_name) (__net_l2_##_name)
#define NET_L2_DECLARE_PUBLIC(_name)					\
	extern const struct net_l2 NET_L2_GET_NAME(_name)
#define NET_L2_GET_CTX_TYPE(_name) _name##_CTX_TYPE

extern struct net_l2 __net_l2_start[];

#ifdef CONFIG_NET_L2_DUMMY
#define DUMMY_L2		DUMMY
#define DUMMY_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(DUMMY_L2);
#endif /* CONFIG_NET_L2_DUMMY */

#ifdef CONFIG_NET_L2_ETHERNET
#define ETHERNET_L2		ETHERNET
#define ETHERNET_L2_CTX_TYPE	void*
NET_L2_DECLARE_PUBLIC(ETHERNET_L2);
#endif /* CONFIG_NET_L2_ETHERNET */

#ifdef CONFIG_NET_L2_IEEE802154
#include <net/ieee802154.h>
#define IEEE802154_L2		IEEE802154
#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context
NET_L2_DECLARE_PUBLIC(IEEE802154_L2);
#endif /* CONFIG_NET_L2_IEEE802154 */

#ifdef CONFIG_NET_L2_BLUETOOTH
#define BLUETOOTH_L2		BLUETOOTH
#define BLUETOOTH_L2_CTX_TYPE	void*
#endif /* CONFIG_NET_L2_BLUETOOTH */

#ifdef CONFIG_NET_L2_OFFLOAD_IP
#define OFFLOAD_IP_L2		OFFLOAD_IP
#define OFFLOAD_IP_L2_CTX_TYPE	void*
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

extern struct net_l2 __net_l2_end[];

#define NET_L2_INIT(_name, _recv_fn, _send_fn, _reserve_fn, _enable_fn)	\
	const struct net_l2 (NET_L2_GET_NAME(_name)) __used		\
	__attribute__((__section__(".net_l2.init"))) = {		\
		.recv = (_recv_fn),					\
		.send = (_send_fn),					\
		.reserve = (_reserve_fn),				\
		.enable = (_enable_fn),					\
	}

#define NET_L2_GET_DATA(name, sfx) (__net_l2_data_##name##sfx)

#define NET_L2_DATA_INIT(name, sfx, ctx_type)				\
	static ctx_type NET_L2_GET_DATA(name, sfx) __used		\
	__attribute__((__section__(".net_l2.data")));

#ifdef __cplusplus
}
#endif

#endif /* __NET_L2_H__ */
