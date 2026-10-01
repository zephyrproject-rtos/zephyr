/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_slip_eth

#define LOG_MODULE_NAME eth_slip_tap
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/ethernet.h>
#include "../net/slip.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one enabled zephyr,slip-eth node is needed");

struct eth_slip_tap_config {
	/* slip_config must be first */
	struct slip_config slip;
	struct net_eth_mac_config mac_cfg;
};

static void eth_slip_tap_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_slip_tap_config *cfg = dev->config;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];

	ethernet_init(iface);
	slip_iface_init(iface);

	if (net_eth_mac_load(&cfg->mac_cfg, mac_addr) == 0) {
		net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	}
}

static enum ethernet_hw_caps eth_capabilities(const struct device *dev __unused,
					      struct net_if *iface __unused)
{
	return ETHERNET_HW_VLAN
#if defined(CONFIG_NET_LLDP)
	       | ETHERNET_LLDP
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_slip_tap_set_config(const struct device *dev __unused,
				   struct net_if *iface __unused,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		return 0;
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api slip_if_api = {
	.iface_api.init = eth_slip_tap_iface_init,

	.get_capabilities = eth_capabilities,
	.send = slip_send,
	.set_config = eth_slip_tap_set_config,
};

#define _SLIP_L2_LAYER    ETHERNET_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

static struct slip_context slip_context_data;

static const struct eth_slip_tap_config eth_slip_tap_config = {
	.slip.uart = DEVICE_DT_GET(DT_INST_BUS(0)),
	.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, NULL, NULL, &slip_context_data, &eth_slip_tap_config,
			      CONFIG_ETH_INIT_PRIORITY, &slip_if_api, NET_ETH_MTU);
