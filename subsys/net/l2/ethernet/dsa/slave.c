/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_slave, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/dsa.h>
#include <zephyr/net/ethernet.h>

struct net_if *dsa_slave_get_iface(struct net_if *iface, int slave_num)
{
	struct ethernet_context *eth_ctx;
	struct dsa_context *dsa_ctx;

	eth_ctx = net_if_l2_data(iface);
	if (eth_ctx == NULL) {
		LOG_ERR("Iface %p context not available!", iface);
		return NULL;
	}

	dsa_ctx = eth_ctx->dsa_ctx;

#if defined(CONFIG_NET_DSA_LEGACY)
	if (slave_num < 0 || slave_num >= dsa_ctx->num_slave_ports) {
#else
	if (slave_num < 0 || slave_num >= dsa_ctx->num_ports) {
#endif
		return NULL;
	}

	return dsa_ctx->iface_slave[slave_num];
}
