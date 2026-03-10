/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real ethenet driver. It is used to instantiate struct
 * devices for the "vnd,ethernet" devicetree compatible used in test code.
 */

#include "eth.h"
#include "eth_test_priv.h"

#define DT_DRV_COMPAT vnd_ethernet

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *vnd_ethernet_get_stats(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NULL;
}

#endif
static int vnd_ethernet_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int vnd_ethernet_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static enum ethernet_hw_caps vnd_ethernet_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int vnd_ethernet_set_config(const struct device *dev, enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return -ENOTSUP;
}

static int vnd_ethernet_get_config(const struct device *dev, enum ethernet_config_type type,
				   struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return -ENOTSUP;
}

#if defined(CONFIG_NET_VLAN)
static int vnd_ethernet_vlan_setup(const struct device *dev, struct net_if *iface, uint16_t tag,
			    bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(tag);
	ARG_UNUSED(enable);

	return -ENOTSUP;
}

#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK)
static const struct device *vnd_ethernet_get_ptp_clock(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NULL;
}

#endif /* CONFIG_PTP_CLOCK */
static const struct device *vnd_ethernet_get_phy(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NULL;
}

static int vnd_ethernet_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return -ENOTSUP;
}

static void vnd_ethernet_iface_init(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

static int vnd_ethernet_init(const struct device *dev)
{
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;

	data->mac_addr_load_result = net_eth_mac_load(&cfg->mcfg, data->mac_addr);

	return 0;
}

struct ethernet_api vnd_ethernet_api = {
	.iface_api.init = vnd_ethernet_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = vnd_ethernet_get_stats,
#endif
	.start = vnd_ethernet_start,
	.stop = vnd_ethernet_stop,
	.get_capabilities = vnd_ethernet_get_capabilities,
	.set_config = vnd_ethernet_set_config,
	.get_config = vnd_ethernet_get_config,
#if defined(CONFIG_NET_VLAN)
	.vlan_setup = vnd_ethernet_vlan_setup,
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK)
	.get_ptp_clock = vnd_ethernet_get_ptp_clock,
#endif /* CONFIG_PTP_CLOCK */
	.get_phy = vnd_ethernet_get_phy,
	.send = vnd_ethernet_send,
};

#define VND_ETHERNET_INIT(n)                                                                       \
	static const struct vnd_ethernet_config vnd_ethernet_cfg_##n = {                           \
		.mcfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                        \
	};                                                                                         \
	static struct vnd_ethernet_data vnd_ethernet_data_##n;                                     \
	DEVICE_DT_INST_DEFINE(n, vnd_ethernet_init, NULL, &vnd_ethernet_data_##n,                  \
			      &vnd_ethernet_cfg_##n, POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,        \
			      &vnd_ethernet_api);

DT_INST_FOREACH_STATUS_OKAY(VND_ETHERNET_INIT)
