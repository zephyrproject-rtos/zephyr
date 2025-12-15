/*
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_cobs, CONFIG_NET_L2_COBS_SERIAL_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dummy.h>

#include "../../ip/net_private.h"
#include "../../ip/net_stats.h"

static enum net_verdict cobs_serial_recv(struct net_if *iface,
					 struct net_pkt *pkt)
{
	if (!pkt || !pkt->buffer) {
		return NET_DROP;
	}

	LOG_DBG("received packet %p on iface %p, len %zu (encoded)",
		pkt, iface, net_pkt_get_len(pkt));


	/* Self-loop helper: when a device routes traffic through a physical
	 * serial medium to itself (two ifaces on same node), IPv6 input can
	 * drop packets because the source address is local ("src addr is mine").
	 * Marking the packet as loopback skips that anti-spoofing check.
	 */
	if (IS_ENABLED(CONFIG_NET_L2_COBS_SERIAL_RX_MARK_LOOPBACK)) {
		net_pkt_set_loopback(pkt, true);
	}

	return NET_CONTINUE;
}

static int cobs_serial_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct dummy_api *api = net_if_get_device(iface)->api;
	int ret;

	if (!pkt || !pkt->buffer) {
		return -ENODATA;
	}

	if (!api) {
		return -ENOENT;
	}

	LOG_DBG("Sending packet %p on iface %p, len %zu",
		pkt, iface, net_pkt_get_len(pkt));

	/* Forward directly to device driver (not back through net_send_data!) */
	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (!ret) {
		size_t pkt_len = net_pkt_get_len(pkt);

		LOG_DBG("Sent pkt %p len %zu", pkt, pkt_len);
#if defined(CONFIG_NET_STATISTICS)
		net_stats_update_bytes_sent(iface, pkt_len);
#endif

		ret = (int)pkt_len;
		net_pkt_unref(pkt);
	}

	return ret;
}

static int cobs_serial_enable(struct net_if *iface, bool state)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(state);

	/* Nothing special to do */
	return 0;
}

static enum net_l2_flags cobs_serial_flags(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NET_L2_MULTICAST | NET_L2_POINT_TO_POINT;
}

NET_L2_INIT(COBS_SERIAL_L2, cobs_serial_recv, cobs_serial_send,
	    cobs_serial_enable, cobs_serial_flags);

