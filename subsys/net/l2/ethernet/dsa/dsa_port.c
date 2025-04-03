/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa_port, CONFIG_NET_DSA_LOG_LEVEL);

#include <zephyr/net/dsa_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#ifdef CONFIG_NET_L2_ETHERNET
bool dsa_port_is_conduit(struct net_if *iface)
{
	/* First check if iface points to ETH interface */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		/* Check its capabilities */
		if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_CONDUIT_PORT) {
			return true;
		}
	}

	return false;
}
#else
bool dsa_port_is_conduit(struct net_if *iface)
{
	return false;
}
#endif

int dsa_port_initialize(const struct device *dev)
{
#if defined(CONFIG_PINCTRL)
	const struct dsa_port_config *cfg = dev->config;
#endif
	struct dsa_context *dsa_ctx = dev->data;
	int err = 0;

	dsa_ctx->init_ports++;

#if defined(CONFIG_PINCTRL)
	if (cfg->pincfg) {
		err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			goto out;
		}
	}
#endif
	if (dsa_ctx->dapi->port_init) {
		err = dsa_ctx->dapi->port_init(dev);
		if (err) {
			goto out;
		}
	}

out:
	/* All ports are initialized. May need switch setup. */
	if (dsa_ctx->init_ports == dsa_ctx->num_ports) {
		if (dsa_ctx->dapi->switch_setup) {
			err = dsa_ctx->dapi->switch_setup(dsa_ctx);
		}
	}

	return err;
}

static void dsa_port_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_port_config *cfg = (struct dsa_port_config *)dev->config;
	struct dsa_context *dsa_ctx = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct ethernet_context *eth_ctx_conduit = NULL;
	int i = cfg->port_idx;
	char name[10];

	/* Find conduit port */
	if (!dsa_ctx->iface_conduit && cfg->ethernet_connection) {
		dsa_ctx->iface_conduit = net_if_lookup_by_dev(cfg->ethernet_connection);
		if (dsa_ctx->iface_conduit == NULL) {
			LOG_ERR("DSA: Conduit iface NOT found!");
			return;
		}

		/*
		 * Provide pointer to DSA context to conduit's eth interface
		 * struct ethernet_context
		 */
		eth_ctx_conduit = net_if_l2_data(dsa_ctx->iface_conduit);
		eth_ctx_conduit->dsa_ctx = dsa_ctx;
		eth_ctx_conduit->dsa_port = DSA_CONDUIT_PORT;
	}

	/* set interface name */
	sprintf(name, "swp%d", i);
	net_if_set_name(iface, name);

	/* Use random mac address if could */
	if (cfg->use_random_mac_addr && dsa_ctx->dapi->port_generate_random_mac) {
		dsa_ctx->dapi->port_generate_random_mac(cfg->mac_addr);
	}
	net_if_set_link_addr(iface, cfg->mac_addr, sizeof(cfg->mac_addr),
			     NET_LINK_ETHERNET);

	if (strncmp(cfg->phy_mode, "internal", sizeof("internal") - 1) == 0) {
		eth_ctx->dsa_port = DSA_CPU_PORT;
		net_if_carrier_off(iface);
		return;
	}

	if (!eth_ctx_conduit && !dsa_ctx->iface_user[i]) {
		dsa_ctx->iface_user[i] = iface;
		eth_ctx->dsa_port_idx = i;
		eth_ctx->dsa_ctx = dsa_ctx;
		eth_ctx->dsa_port = DSA_USER_PORT;

		/*
		 * Initialize ethernet context 'work' for this iface to
		 * be able to monitor the carrier status.
		 */
		ethernet_init(iface);
	}

	/* Do not start the interface until link is up */
	net_if_carrier_off(iface);

	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface", cfg->phy_dev);
		return;
	}

	if (!dsa_ctx->dapi->port_phylink_change) {
		LOG_ERR("require port_phylink_change callback");
		return;
	}

	phy_link_callback_set(cfg->phy_dev, dsa_ctx->dapi->port_phylink_change, (void *)dev);
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
