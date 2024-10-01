/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Multicast Listener Discovery API
 */

#ifndef ZEPHYR_INCLUDE_NET_MLD_H_
#define ZEPHYR_INCLUDE_NET_MLD_H_

/**
 * @brief MLD (Multicast Listener Discovery)
 * @defgroup mld Multicast Listener Discovery API
 * @since 1.8
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Join a given multicast group.
 *
 * @param iface Network interface where join message is sent
 * @param addr Multicast group to join
 *
 * @return 0 if joining was done, <0 otherwise.
 */
#if defined(CONFIG_NET_IPV6_MLD)
int net_ipv6_mld_join(struct net_if *iface, const struct in6_addr *addr);
#else
static inline int
net_ipv6_mld_join(struct net_if *iface, const struct in6_addr *addr)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(iface);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6_MLD */

/**
 * @brief Leave a given multicast group.
 *
 * @param iface Network interface where leave message is sent
 * @param addr Multicast group to leave
 *
 * @return 0 if leaving is done, <0 otherwise.
 */
#if defined(CONFIG_NET_IPV6_MLD)
int net_ipv6_mld_leave(struct net_if *iface, const struct in6_addr *addr);
#else
static inline int
net_ipv6_mld_leave(struct net_if *iface, const struct in6_addr *addr)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(addr);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6_MLD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_MLD_H_ */
