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

struct eth_tsn_nic_config {
};

struct eth_tsn_nic_data {
};

static int eth_tsn_nic_start(const struct device *dev) {
    return 0;
}


static const struct ethernet_api eth_tsn_nic_api = {
    .start = eth_tsn_nic_start,
};

static int eth_tsn_nic_init(const struct device *dev) {
    printk("Hi\n");
    return 0;
}

/* TODO: priority should be CONFIG_ETH_INIT_PRIORITY*/
#define ETH_TSN_NIC_INIT(n)                                                                         \
	static struct eth_tsn_nic_data eth_tsn_nic_data_##n = {};                                    \
                                                                                                   \
	static const struct eth_tsn_nic_config eth_tsn_nic_cfg_##n = {                               \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_tsn_nic_init, NULL, &eth_tsn_nic_data_##n,              \
				      &eth_tsn_nic_cfg_##n, 99,            \
				      &eth_tsn_nic_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_TSN_NIC_INIT)
