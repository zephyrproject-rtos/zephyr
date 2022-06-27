/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IGMP API
 */

#ifndef ZEPHYR_INCLUDE_NET_IGMP_H_
#define ZEPHYR_INCLUDE_NET_IGMP_H_

/**
 * @brief IGMP (Internet Group Management Protocol)
 * @defgroup igmp IGMP API
 * @ingroup networking
 * @{
 */

#include <zephyr/types.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Join a given multicast group.
 *
 * @param iface Network interface where join message is sent
 * @param addr Multicast group to join
 *
 * @return Return 0 if joining was done, <0 otherwise.
 */
#if defined(CONFIG_NET_IPV4_IGMP)
int net_ipv4_igmp_join(struct net_if *iface, const struct in_addr *addr);
#else
static inline int net_ipv4_igmp_join(struct net_if *iface,
				     const struct in_addr *addr)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(addr);

	return -ENOSYS;
}
#endif

/**
 * @brief Leave a given multicast group.
 *
 * @param iface Network interface where leave message is sent
 * @param addr Multicast group to leave
 *
 * @return Return 0 if leaving is done, <0 otherwise.
 */
#if defined(CONFIG_NET_IPV4_IGMP)
int net_ipv4_igmp_leave(struct net_if *iface, const struct in_addr *addr);
#else
static inline int net_ipv4_igmp_leave(struct net_if *iface,
				      const struct in_addr *addr)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(addr);

	return -ENOSYS;
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IGMP_H_ */
