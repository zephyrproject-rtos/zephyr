/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_port, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/dsa_core.h>
#include "dsa_tag.h"

#if defined(CONFIG_NET_INTERFACE_NAME_LEN)
#define INTERFACE_NAME_LEN CONFIG_NET_INTERFACE_NAME_LEN
#else
#define INTERFACE_NAME_LEN 10
#endif

int dsa_port_initialize(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct ethernet_context *eth_ctx_conduit = NULL;
	int err = 0;

	dsa_switch_ctx->init_ports++;

	/* Find the connection of conduit port and cpu port */
	if (dsa_switch_ctx->iface_conduit == NULL && cfg->ethernet_connection != NULL) {
		dsa_switch_ctx->iface_conduit = net_if_lookup_by_dev(cfg->ethernet_connection);
		if (dsa_switch_ctx->iface_conduit == NULL) {
			LOG_ERR("DSA: Conduit iface NOT found!");
		}

		/* Set up tag protocol on the cpu port */
		eth_ctx->dsa_port = DSA_CPU_PORT;
		dsa_tag_setup(dev);

		/* Provide DSA information to the conduit port */
		eth_ctx_conduit = net_if_l2_data(dsa_switch_ctx->iface_conduit);
		eth_ctx_conduit->dsa_switch_ctx = dsa_switch_ctx;
		eth_ctx_conduit->dsa_port = DSA_CONDUIT_PORT;
	}

	if (cfg->ethernet_connection == NULL) {
		eth_ctx->dsa_port = DSA_USER_PORT;
		eth_ctx->dsa_switch_ctx = dsa_switch_ctx;
		dsa_switch_ctx->iface_user[cfg->port_idx] = iface;
	}

	if (dsa_switch_ctx->dapi->port_init != NULL) {
		err = dsa_switch_ctx->dapi->port_init(dev);
		if (err != 0) {
			goto out;
		}
	}

out:
	/* All ports are initialized. May need switch setup. */
	if (dsa_switch_ctx->init_ports == dsa_switch_ctx->num_ports) {
		if (dsa_switch_ctx->dapi->switch_setup != NULL) {
			err = dsa_switch_ctx->dapi->switch_setup(dsa_switch_ctx);
		}
	}

	return err;
}

static void dsa_port_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_port_config *cfg = (struct dsa_port_config *)dev->config;
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	char name[INTERFACE_NAME_LEN];

	/* Set interface name */
	snprintf(name, sizeof(name), "swp%d", cfg->port_idx);
	net_if_set_name(iface, name);

	/* Use random mac address if could */
	if (cfg->use_random_mac_addr && dsa_switch_ctx->dapi->port_generate_random_mac != NULL) {
		dsa_switch_ctx->dapi->port_generate_random_mac(cfg->mac_addr);
	}

	net_if_set_link_addr(iface, cfg->mac_addr, sizeof(cfg->mac_addr), NET_LINK_ETHERNET);

	if (cfg->ethernet_connection != NULL) {
		net_if_carrier_off(iface);
		return;
	}

	/*
	 * Initialize ethernet context 'work' for this iface to
	 * be able to monitor the carrier status.
	 */
	ethernet_init(iface);

	/* Do not start the interface until link is up */
	net_if_carrier_off(iface);

	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface", cfg->phy_dev);
		return;
	}

	if (dsa_switch_ctx->dapi->port_phylink_change == NULL) {
		LOG_ERR("require port_phylink_change callback");
		return;
	}

	phy_link_callback_set(cfg->phy_dev, dsa_switch_ctx->dapi->port_phylink_change, (void *)dev);
}

static const struct device *dsa_port_get_phy(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;

	return cfg->phy_dev;
}

const struct ethernet_api dsa_eth_api = {
	.iface_api.init = dsa_port_iface_init,
	.get_phy = dsa_port_get_phy,
	.send = dsa_xmit,
};
