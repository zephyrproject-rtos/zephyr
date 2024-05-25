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
	NET_IPV4_AUTOCONF_PROBE,    /**< Probing state */
	NET_IPV4_AUTOCONF_ANNOUNCE, /**< Announce state */
	NET_IPV4_AUTOCONF_ASSIGNED, /**< Assigned state */
	NET_IPV4_AUTOCONF_RENEW,    /**< Renew state */
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initialize IPv4 auto configuration engine.
 */
#if defined(CONFIG_NET_IPV4_AUTO)
void net_ipv4_autoconf_init(void);
#else
#define net_ipv4_autoconf_init(...)
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_NET_IPV4_AUTOCONF_H_ */
