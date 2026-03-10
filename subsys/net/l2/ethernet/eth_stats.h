/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ETH_STATS_H__
#define __ETH_STATS_H__

#if defined(CONFIG_NET_STATISTICS_ETHERNET)

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

static inline struct net_stats_eth *eth_stats_get_common(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;

	if (api->get_stats_type != NULL) {
		return api->get_stats_type(net_if_get_device(iface),
					   ETHERNET_STATS_TYPE_COMMON);
	} else if (api->get_stats != NULL) {
		return api->get_stats(net_if_get_device(iface));
	}

	return NULL;
}

static inline void eth_stats_update_bytes_rx(struct net_if *iface, uint32_t bytes)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->bytes.received += bytes;
	}
}

static inline void eth_stats_update_bytes_tx(struct net_if *iface, uint32_t bytes)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->bytes.sent += bytes;
	}
}

static inline void eth_stats_update_pkts_rx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->pkts.rx++;
	}
}

static inline void eth_stats_update_pkts_tx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->pkts.tx++;
	}
}

static inline void eth_stats_update_broadcast_rx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->broadcast.rx++;
	}
}

static inline void eth_stats_update_broadcast_tx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->broadcast.tx++;
	}
}

static inline void eth_stats_update_multicast_rx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->multicast.rx++;
	}
}

static inline void eth_stats_update_multicast_tx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->multicast.tx++;
	}
}

static inline void eth_stats_update_errors_rx(struct net_if *iface)
{
	struct net_stats_eth *stats;

	if (!iface) {
		return;
	}

	stats = eth_stats_get_common(iface);
	if (stats != NULL) {
		stats->errors.rx++;
	}
}

static inline void eth_stats_update_errors_tx(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->errors.tx++;
	}
}

static inline void eth_stats_update_unknown_protocol(struct net_if *iface)
{
	struct net_stats_eth *stats = eth_stats_get_common(iface);

	if (stats != NULL) {
		stats->unknown_protocol++;
	}
}

#else /* CONFIG_NET_STATISTICS_ETHERNET */

#define eth_stats_update_bytes_rx(iface, bytes)
#define eth_stats_update_bytes_tx(iface, bytes)
#define eth_stats_update_pkts_rx(iface)
#define eth_stats_update_pkts_tx(iface)
#define eth_stats_update_broadcast_rx(iface)
#define eth_stats_update_broadcast_tx(iface)
#define eth_stats_update_multicast_rx(iface)
#define eth_stats_update_multicast_tx(iface)
#define eth_stats_update_errors_rx(iface)
#define eth_stats_update_errors_tx(iface)
#define eth_stats_update_unknown_protocol(iface)

#endif /* CONFIG_NET_STATISTICS_ETHERNET */

#endif /* __ETH_STATS_H__ */
