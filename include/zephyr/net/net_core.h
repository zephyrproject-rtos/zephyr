/** @file
 * @brief Network core definitions
 *
 * Definitions for networking support.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CORE_H_
#define ZEPHYR_INCLUDE_NET_NET_CORE_H_

#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>

#include <zephyr/net/net_timeout.h>
#include <zephyr/net/net_linkaddr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Networking
 * @defgroup networking Networking
 * @since 1.0
 * @version 1.0.0
 * @ingroup connectivity
 * @{
 * @}
 */

/**
 * @brief Network core library
 * @defgroup net_core Network Core Library
 * @since 1.0
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Network subsystem logging helpers */
#ifdef CONFIG_THREAD_NAME
#define NET_DBG(fmt, ...) LOG_DBG("(%s): " fmt,				\
			k_thread_name_get(k_current_get()), \
			##__VA_ARGS__)
#else
#define NET_DBG(fmt, ...) LOG_DBG("(%p): " fmt, k_current_get(),	\
				  ##__VA_ARGS__)
#endif /* CONFIG_THREAD_NAME */
#define NET_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_INFO(fmt, ...) LOG_INF(fmt,  ##__VA_ARGS__)

#define NET_HEXDUMP_DBG(_data, _length, _str) LOG_HEXDUMP_DBG(_data, _length, _str)
#define NET_HEXDUMP_ERR(_data, _length, _str) LOG_HEXDUMP_ERR(_data, _length, _str)
#define NET_HEXDUMP_WARN(_data, _length, _str) LOG_HEXDUMP_WRN(_data, _length, _str)
#define NET_HEXDUMP_INFO(_data, _length, _str) LOG_HEXDUMP_INF(_data, _length, _str)

#define NET_ASSERT(cond, ...) __ASSERT(cond, "" __VA_ARGS__)

/* This needs to be here in order to avoid circular include dependency between
 * net_pkt.h and net_if.h
 */
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) || \
	defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
#if !defined(NET_PKT_DETAIL_STATS_COUNT)
#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)

#if defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
#define NET_PKT_DETAIL_STATS_COUNT 4
#else
#define NET_PKT_DETAIL_STATS_COUNT 3
#endif /* CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

#else
#define NET_PKT_DETAIL_STATS_COUNT 4
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL */

#endif /* !NET_PKT_DETAIL_STATS_COUNT */
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL ||
	  CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

/** @endcond */

struct net_buf;
struct net_pkt;
struct net_context;
struct net_if;

/**
 * @brief Net Verdict
 */
enum net_verdict {
	/** Packet has been taken care of. */
	NET_OK,
	/** Packet has not been touched, other part should decide about its
	 * fate.
	 */
	NET_CONTINUE,
	/** Packet must be dropped. */
	NET_DROP,
};

/**
 * @brief Called by lower network stack or network device driver when
 * a network packet has been received. The function will push the packet up in
 * the network stack for further processing.
 *
 * @param iface Network interface where the packet was received.
 * @param pkt Network packet data.
 *
 * @return 0 if ok, <0 if error.
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Try sending data to network.
 *
 * @details Send data to network. This should not be used normally by
 * applications as it requires that the network packet is properly
 * constructed.
 *
 * @param pkt Network packet.
 * @param timeout Timeout for send.
 *
 * @return 0 if ok, <0 if error. If <0 is returned, then the caller needs
 * to unref the pkt in order to avoid memory leak.
 */
int net_try_send_data(struct net_pkt *pkt, k_timeout_t timeout);

/**
 * @brief Send data to network.
 *
 * @details Send data to network. This should not be used normally by
 * applications as it requires that the network packet is properly
 * constructed. Equivalent to net_try_send_data with infinite timeout.
 *
 * @param pkt Network packet.
 *
 * @return 0 if ok, <0 if error. If <0 is returned, then the caller needs
 * to unref the pkt in order to avoid memory leak.
 */
static inline int net_send_data(struct net_pkt *pkt)
{
	k_timeout_t timeout = k_is_in_isr() ? K_NO_WAIT : K_FOREVER;

	return net_try_send_data(pkt, timeout);
}

/** @cond INTERNAL_HIDDEN */

/* Some helper defines for traffic class support */
#if defined(CONFIG_NET_TC_TX_COUNT) && defined(CONFIG_NET_TC_RX_COUNT)
#define NET_TC_TX_COUNT CONFIG_NET_TC_TX_COUNT
#define NET_TC_RX_COUNT CONFIG_NET_TC_RX_COUNT

#if NET_TC_TX_COUNT > NET_TC_RX_COUNT
#define NET_TC_COUNT NET_TC_TX_COUNT
#else
#define NET_TC_COUNT NET_TC_RX_COUNT
#endif
#else /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */
#define NET_TC_TX_COUNT 0
#define NET_TC_RX_COUNT 0
#define NET_TC_COUNT 0
#endif /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */

/**
 * @brief Registration information for a given L3 handler. Note that
 *        the layer number (L3) just refers to something that is on top
 *        of L2. So for example IPv6 is L3 and IPv4 is L3, but Ethernet
 *        based LLDP, gPTP are more in the layer 2.5 but we consider them
 *        as L3 here for simplicity.
 */
struct net_l3_register {
	/** Store also the name of the L3 type in order to be able to
	 * print it later.
	 */
	const char * const name;
	/** What L2 layer this is for */
	const struct net_l2 * const l2;
	/** Handler function for the specified protocol type. If the handler
	 * has taken ownership of the pkt, it must return NET_OK. If it wants to
	 * continue processing at the next level (e.g. ipv4), it must return
	 * NET_CONTINUE. If instead something is wrong with the packet (for
	 * example, a multicast address that does not match the protocol type)
	 * it must return NET_DROP so that the statistics can be updated
	 * accordingly
	 */
	enum net_verdict (*handler)(struct net_if *iface,
				    uint16_t ptype,
				    struct net_pkt *pkt);
	/** Protocol type */
	uint16_t ptype;
};

#define NET_L3_GET_NAME(l3_name, ptype) __net_l3_register_##l3_name##_##ptype

#define NET_L3_REGISTER(_l2_type, _name, _ptype, _handler)		\
	static const STRUCT_SECTION_ITERABLE(net_l3_register,		\
				    NET_L3_GET_NAME(_name, _ptype)) = { \
		.ptype = _ptype,					\
		.handler = _handler,					\
		.name = STRINGIFY(_name),				\
		.l2 = _l2_type,						\
	};

/* @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CORE_H_ */
