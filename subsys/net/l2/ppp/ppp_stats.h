/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PPP_STATS_H__
#define __PPP_STATS_H__

#if defined(CONFIG_NET_STATISTICS_PPP)

#include <net/net_ip.h>
#include <net/net_stats.h>
#include <net/net_if.h>

static inline void ppp_stats_update_bytes_rx(struct net_if *iface,
					     uint32_t bytes)
{
	const struct ppp_api *api = (const struct ppp_api *)
		net_if_get_device(iface)->api;
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->bytes.received += bytes;
}

static inline void ppp_stats_update_bytes_tx(struct net_if *iface,
					     uint32_t bytes)
{
	const struct ppp_api *api = (const struct ppp_api *)
		net_if_get_device(iface)->api;
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->bytes.sent += bytes;
}

static inline void ppp_stats_update_pkts_rx(struct net_if *iface)
{
	const struct ppp_api *api = (const struct ppp_api *)
		net_if_get_device(iface)->api;
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->pkts.rx++;
}

static inline void ppp_stats_update_pkts_tx(struct net_if *iface)
{
	const struct ppp_api *api = (const struct ppp_api *)
		net_if_get_device(iface)->api;
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->pkts.tx++;
}

static inline void ppp_stats_update_drop_rx(struct net_if *iface)
{
	const struct ppp_api *api = ((const struct ppp_api *)
		net_if_get_device(iface)->api);
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->drop++;
}

static inline void ppp_stats_update_fcs_error_rx(struct net_if *iface)
{
	const struct ppp_api *api = ((const struct ppp_api *)
		net_if_get_device(iface)->api);
	struct net_stats_ppp *stats;

	if (!api->get_stats) {
		return;
	}

	stats = api->get_stats(net_if_get_device(iface));
	if (!stats) {
		return;
	}

	stats->chkerr++;
}

#else /* CONFIG_NET_STATISTICS_PPP */

#define ppp_stats_update_bytes_rx(iface, bytes)
#define ppp_stats_update_bytes_tx(iface, bytes)
#define ppp_stats_update_pkts_rx(iface)
#define ppp_stats_update_pkts_tx(iface)
#define ppp_stats_update_drop_rx(iface)
#define ppp_stats_update_fcs_error_rx(iface)

#endif /* CONFIG_NET_STATISTICS_PPP */

#endif /* __PPP_STATS_H__ */
