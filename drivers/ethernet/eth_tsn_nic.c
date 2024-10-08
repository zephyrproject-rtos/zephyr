/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Better name */
#define DT_DRV_COMPAT tsnlab_tsn_nic_eth

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_tsn_nic, LOG_LEVEL_ERR);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ethernet.h>

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/dma.h>

struct eth_tsn_nic_config {
	const struct device *pci_dev;
	const struct device *dma_dev;
};

struct eth_tsn_nic_data {
	struct net_if *iface;
};

static void eth_tsn_nic_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_tsn_nic_config *config = dev->config;
	struct eth_tsn_nic_data *data = dev->data;

	if (data->iface == NULL) {
		data->iface = iface;
	}

	ethernet_init(iface);

	ARG_UNUSED(config);

	/* TODO: sw-238 (Setup) */
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *get_stats(const struct device *dev)
{
	/* TODO: sw-257 (Misc. APIs) */
	return -ENOTSUP;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static int eth_tsn_nic_start(const struct device *dev)
{
	/* TODO: sw-238 (Setup) */
	return -ENOTSUP;
}

static int eth_tsn_nic_stop(const struct device *dev)
{
	/* TODO: sw-257 (Misc. APIs) */
	return -ENOTSUP;
}

static enum ethernet_hw_caps eth_tsn_nic_get_capabilities(const struct device *dev)
{
	/* TODO: sw-257 (Misc. APIs) */
	return -ENOTSUP;
}

static int eth_tsn_nic_set_config(const struct device *dev, enum ethernet_config_type type,
				  const struct ethernet_config *config)
{
	/* TODO: sw-295 (QoS) */
	return -ENOTSUP;
}

static int eth_tsn_nic_get_config(const struct device *dev, enum ethernet_config_type type,
				  struct ethernet_config *config)
{
	/* TODO: sw-295 (QoS) */
	return -ENOTSUP;
}

#if defined(CONFIG_NET_VLAN)
static int eth_tsn_nic_vlan_setup(const struct device *dev, struct net_if *iface, uint16_t tag,
				  bool enable)
{
	/* TODO: sw-257 (Misc. APIs) or a new issue */
	return -ENOTSUP;
}
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK)
static const struct device *eth_tsn_nic_get_ptp_clock(const struct device *dev)
{
	/* TODO: sw-290 (PTP) */
	return NULL;
}
#endif /* CONFIG_PTP_CLOCK */

static const struct device *eth_tsn_nic_get_phy(const struct device *dev)
{
	/* TODO: sw-257 (Misc. APIs) */
	/* This might not be needed at all */
	return NULL;
}

static int eth_tsn_nic_send(const struct device *dev, struct net_pkt *pkt)
{
	/* TODO: sw-240 (Tx) */
	return -ENOTSUP;
}

static const struct ethernet_api eth_tsn_nic_api = {
	.iface_api.init = eth_tsn_nic_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_tsn_nic_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.start = eth_tsn_nic_start,
	.stop = eth_tsn_nic_stop,
	.get_capabilities = eth_tsn_nic_get_capabilities,
	.set_config = eth_tsn_nic_set_config,
	.get_config = eth_tsn_nic_get_config,
#if defined(CONFIG_NET_VLAN)
	.vlan_setup = eth_tsn_nic_vlan_setup,
#endif /* CONFIG_NET_VLAN */
#if defined(CONFIG_PTP_CLOCK)
	.get_ptp_clock = eth_tsn_nic_get_ptp_clock,
#endif /* CONFIG_PTP_CLOCK */
	.get_phy = eth_tsn_nic_get_phy,
	.send = eth_tsn_nic_send,
};

static int eth_tsn_nic_init(const struct device *dev)
{
	const struct eth_tsn_nic_config *config = dev->config;
	mm_reg_t test_dma_addr;

	printk("Ethernet init\n");

	/* Test logs */
	device_map(&test_dma_addr, 0x1b08000000, 0x4000, K_MEM_CACHE_NONE);
	printk("H2C engine id: 0x%x\n", sys_read32(test_dma_addr));
	printk("H2C engine status: 0x%x\n", sys_read32(test_dma_addr + 0x0004));
	dma_start(config->dma_dev, 0);
	printk("H2C engine start\n");
	printk("H2C engine status: 0x%x\n", sys_read32(test_dma_addr + 0x0004));

	/* TODO: sw-238 (Setup) */
	/* Check xdma_netdev_open() */
	return 0;
}

/* TODO: priority should be CONFIG_ETH_INIT_PRIORITY */
#define ETH_TSN_NIC_INIT(n)                                                                        \
	static struct eth_tsn_nic_data eth_tsn_nic_data_##n = {};                                  \
                                                                                                   \
	static const struct eth_tsn_nic_config eth_tsn_nic_cfg_##n = {                             \
		.dma_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
		.pci_dev = DEVICE_DT_GET(DT_GPARENT(DT_DRV_INST(n))),                              \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_tsn_nic_init, NULL, &eth_tsn_nic_data_##n,            \
				      &eth_tsn_nic_cfg_##n, 99, &eth_tsn_nic_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_TSN_NIC_INIT)
