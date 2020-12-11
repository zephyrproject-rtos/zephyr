/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ETH_STATS_H__
#define __ETH_STATS_H__

#if defined(CONFIG_NET_STATISTICS_ETHERNET)

#include <net/net_ip.h>
#include <net/net_stats.h>
#include <net/net_if.h>

static inline void eth_stats_update_bytes_rx(struct net_if *iface,
					     uint32_t bytes)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->bytes.received += bytes;
}

static inline void eth_stats_update_bytes_tx(struct net_if *iface,
					     uint32_t bytes)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->bytes.sent += bytes;
}

static inline void eth_stats_update_pkts_rx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->pkts.rx++;
}

static inline void eth_stats_update_pkts_tx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->pkts.tx++;
}

static inline void eth_stats_update_broadcast_rx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->broadcast.rx++;
}

static inline void eth_stats_update_broadcast_tx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->broadcast.tx++;
}

static inline void eth_stats_update_multicast_rx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->multicast.rx++;
}

static inline void eth_stats_update_multicast_tx(struct net_if *iface)
{
	const struct ethernet_api *api = (const struct ethernet_api *)
		net_if_get_device(iface)->api;
	struct net_stats_eth *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->multicast.tx++;
}


static inline void eth_stats_update_errors_rx(struct net_if *iface)
{
	struct net_stats_eth *stats;
	const struct ethernet_api *api;

	if (!iface) {
		return;
	}

	api = ((const struct ethernet_api *)
	       net_if_get_device(iface)->api);

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->errors.rx++;
}

static inline void eth_stats_update_errors_tx(struct net_if *iface)
{
	struct net_stats_eth *stats;
	const struct ethernet_api *api = ((const struct ethernet_api *)
		net_if_get_device(iface)->api);

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->errors.tx++;
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

#endif /* CONFIG_NET_STATISTICS_ETHERNET */

#endif /* __ETH_STATS_H__ */
