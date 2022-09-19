/*
 * Copyright (c) 2017 Matthias Boesl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IPv4 Autoconfiguration
 *
 * This is not to be included by the application.
 */

#ifndef __IPV4_AUTOCONF_INTERNAL_H
#define __IPV4_AUTOCONF_INTERNAL_H

#include <zephyr/kernel.h>

#include <zephyr/net/ipv4_autoconf.h>

/* Initial random delay*/
#define IPV4_AUTOCONF_PROBE_WAIT 1

/* Number of probe packets */
#define IPV4_AUTOCONF_PROBE_NUM 3

/* Minimum delay till repeated probe */
#define IPV4_AUTOCONF_PROBE_MIN 1

/* Maximum delay till repeated probe */
#define IPV4_AUTOCONF_PROBE_MAX 2

/* Number of announcement packets */
#define IPV4_AUTOCONF_ANNOUNCE_NUM 2

/* Time between announcement packets */
#define IPV4_AUTOCONF_ANNOUNCE_INTERVAL 2

/* Max conflicts before rate limiting */
#define IPV4_AUTOCONF_MAX_CONFLICTS 10

/* Delay between successive attempts */
#define IPV4_AUTOCONF_RATE_LIMIT_INTERVAL 60

/* Minimum interval between defensive ARPs */
#define IPV4_AUTOCONF_DEFEND_INTERVAL 10

/* Time between carrier up and first probe */
#define IPV4_AUTOCONF_START_DELAY 3

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
#define net_ipv4_autoconf_start(...)
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
#define net_ipv4_autoconf_reset(...)
#endif

/**
 * @brief Autoconf ARP input message handler.
 *
 * @details Called when ARP message is received when auto is enabled.
 *
 * @param iface A valid pointer on an interface
 * @param pkt Received network packet
 *
 * @return What should be done with packet (drop or accept)
 */
#if defined(CONFIG_NET_IPV4_AUTO)
enum net_verdict net_ipv4_autoconf_input(struct net_if *iface,
					 struct net_pkt *pkt);
#else
#define net_ipv4_autoconf_input(...) NET_CONTINUE
#endif

#endif /* __IPV4_AUTOCONF_INTERNAL_H */
