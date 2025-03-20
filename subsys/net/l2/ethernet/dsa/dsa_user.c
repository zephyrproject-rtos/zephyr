/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_user, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/dsa_core.h>
#include <zephyr/net/ethernet.h>

struct net_if *dsa_user_get_iface(struct net_if *iface, int port_idx)
{
	struct ethernet_context *eth_ctx;
	struct dsa_context *dsa_ctx;

	eth_ctx = net_if_l2_data(iface);
	if (eth_ctx == NULL) {
		LOG_ERR("Iface %p context not available!", iface);
		return NULL;
	}

	dsa_ctx = eth_ctx->dsa_ctx;

	if (port_idx < 0 || port_idx >= dsa_ctx->num_ports) {
		return NULL;
	}

	return dsa_ctx->iface_user[port_idx];
}
