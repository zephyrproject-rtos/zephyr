/** @file
 * @brief Network interface promiscuous mode support
 *
 * An API for applications to start listening network traffic.
 * This requires support from network device driver and from application.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_PROMISCUOUS_H_
#define ZEPHYR_INCLUDE_NET_PROMISCUOUS_H_

/**
 * @brief Promiscuous mode support.
 * @defgroup promiscuous Promiscuous mode
 * @ingroup networking
 * @{
 */

#include <net/net_pkt.h>
#include <net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start to wait received network packets.
 *
 * @param timeout How long to wait before returning.
 *
 * @return Received net_pkt, NULL if not received any packet.
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
struct net_pkt *net_promisc_mode_wait_data(s32_t timeout);
#else
static inline struct net_pkt *net_promisc_mode_wait_data(s32_t timeout)
{
	ARG_UNUSED(timeout);

	return NULL;
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

/**
 * @brief Enable promiscuous mode for a given network interface.
 *
 * @param iface Network interface
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int net_promisc_mode_on(struct net_if *iface);
#else
static inline int net_promisc_mode_on(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

/**
 * @brief Disable promiscuous mode for a given network interface.
 *
 * @param iface Network interface
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int net_promisc_mode_off(struct net_if *iface);
#else
static inline int net_promisc_mode_off(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_PROMISCUOUS_H_ */
