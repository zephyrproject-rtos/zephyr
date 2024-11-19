/** @file
 * @brief IPv4/6 PMTU related functions
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NET_PMTU_H
#define __NET_PMTU_H

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** PTMU destination cache entry */
struct net_pmtu_entry {
	/** Destination address */
	struct net_addr dst;
	/** Last time the PMTU was updated */
	uint32_t last_update;
	/** MTU for this destination address */
	uint16_t mtu;
	/** In use flag */
	bool in_use : 1;
};

/** Get PMTU entry for the given destination address
 *
 * @param dst Destination address
 *
 * @return PMTU entry if found, NULL otherwise
 */
#if defined(CONFIG_NET_PMTU)
struct net_pmtu_entry *net_pmtu_get_entry(const struct sockaddr *dst);
#else
static inline struct net_pmtu_entry *net_pmtu_get_entry(const struct sockaddr *dst)
{
	ARG_UNUSED(dst);

	return NULL;
}
#endif /* CONFIG_NET_PMTU */

/** Get MTU value for the given destination address
 *
 * @param dst Destination address
 *
 * @return MTU value (> 0) if found, <0 otherwise
 */
#if defined(CONFIG_NET_PMTU)
int net_pmtu_get_mtu(const struct sockaddr *dst);
#else
static inline int net_pmtu_get_mtu(const struct sockaddr *dst)
{
	ARG_UNUSED(dst);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PMTU */

/** Update PMTU value for the given destination address
 *
 * @param dst Destination address
 * @param mtu New MTU value
 *
 * @return >0 previous MTU, <0 if error
 */
#if defined(CONFIG_NET_PMTU)
int net_pmtu_update_mtu(const struct sockaddr *dst, uint16_t mtu);
#else
static inline int net_pmtu_update_mtu(const struct sockaddr *dst, uint16_t mtu)
{
	ARG_UNUSED(dst);
	ARG_UNUSED(mtu);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PMTU */

/** Update PMTU entry for the given destination address
 *
 * @param entry PMTU entry
 * @param mtu New MTU value
 *
 * @return >0 previous MTU, <0 if error
 */
#if defined(CONFIG_NET_PMTU)
int net_pmtu_update_entry(struct net_pmtu_entry *entry, uint16_t mtu);
#else
static inline int net_pmtu_update_entry(struct net_pmtu_entry *entry,
					uint16_t mtu)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(mtu);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PMTU */

/**
 * @typedef net_pmtu_cb_t
 * @brief Callback used when traversing PMTU destination cache.
 *
 * @param entry PMTU entry
 * @param user_data User specified data
 */
typedef void (*net_pmtu_cb_t)(struct net_pmtu_entry *entry,
			      void *user_data);

/** Get PMTU destination cache contents
 *
 * @param cb PMTU callback to be called for each cache entry.
 * @param user_data User specific data.
 *
 * @return >=0 number of entries in the PMTU destination cache, <0 if error
 */
#if defined(CONFIG_NET_PMTU)
int net_pmtu_foreach(net_pmtu_cb_t cb, void *user_data);
#else
static inline int net_pmtu_foreach(net_pmtu_cb_t cb, void *user_data)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_PMTU */

/** Initialize PMTU module */
#if defined(CONFIG_NET_PMTU)
void net_pmtu_init(void);
#else
#define net_pmtu_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NET_PMTU_H */
