/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ppp_stats, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ppp.h>

#include "net_stats.h"

#if defined(CONFIG_NET_STATISTICS_USER_API)

static int ppp_stats_get(uint32_t mgmt_request, struct net_if *iface,
			 void *data, size_t len)
{
	size_t len_chk = 0;
	void *src = NULL;
	const struct ppp_api *ppp;

	if (NET_MGMT_GET_COMMAND(mgmt_request) ==
	    NET_REQUEST_STATS_CMD_GET_PPP) {
		if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
			return -ENOENT;
		}

		ppp = net_if_get_device(iface)->api;
		if (ppp->get_stats == NULL) {
			return -ENOENT;
		}

		len_chk = sizeof(struct net_stats_ppp);
		src = ppp->get_stats(net_if_get_device(iface));
	}

	if (len != len_chk || !src) {
		return -EINVAL;
	}

	memcpy(data, src, len);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_PPP,
				  ppp_stats_get);

#endif /* CONFIG_NET_STATISTICS_USER_API */
