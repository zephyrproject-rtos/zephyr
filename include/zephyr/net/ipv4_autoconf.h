/*
 * Copyright (c) 2017 Matthias Boesl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IPv4 Autoconfiguration
 */

#ifndef ZEPHYR_INCLUDE_NET_IPV4_AUTOCONF_H_
#define ZEPHYR_INCLUDE_NET_IPV4_AUTOCONF_H_

/** Current state of IPv4 Autoconfiguration */
enum net_ipv4_autoconf_state {
	NET_IPV4_AUTOCONF_INIT,     /**< Initialization state */
	NET_IPV4_AUTOCONF_ASSIGNED, /**< Assigned state */
	NET_IPV4_AUTOCONF_RENEW,    /**< Renew state */
};

struct net_if;

/**
 * @brief Start IPv4 autoconfiguration RFC 3927: IPv4 Link Local
 *
 * @details Start IPv4 IP autoconfiguration
 *
 * @param iface A valid pointer on an interface
 */
#if defined(CONFIG_NET_IPV4_AUTO)
void net_ipv4_autoconf_start(struct net_if *iface);
#else
static inline void net_ipv4_autoconf_start(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Reset autoconf process
 *
 * @details Reset IPv4 IP autoconfiguration
 *
 * @param iface A valid pointer on an interface
 */
#if defined(CONFIG_NET_IPV4_AUTO)
void net_ipv4_autoconf_reset(struct net_if *iface);
#else
static inline void net_ipv4_autoconf_reset(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initialize IPv4 auto configuration engine.
 */
#if defined(CONFIG_NET_IPV4_AUTO)
void net_ipv4_autoconf_init(void);
#else
static inline void net_ipv4_autoconf_init(void) { }
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_NET_IPV4_AUTOCONF_H_ */
