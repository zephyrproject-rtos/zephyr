/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dwmac_core, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/bit_rev.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include "eth_dwmac_priv.h"

#ifdef CONFIG_ETH_DWC_ETHER_1000_CORE
#define DWMAC_MCAST_HASH_TABLE_REG(i) ((i) == 0 ? DWMAC_MACHTLR : DWMAC_MACHTHR)

BUILD_ASSERT(DWMAC_MULTICAST_FILTER_BINS == 64, "this core only supports 64 multicast hash bins");
#endif
#ifdef CONFIG_ETH_DWC_ETHER_QOS_CORE
#define DWMAC_MCAST_HASH_TABLE_REG(i) MAC_HASH_TABLE(i)
#endif

#define DWMAC_MCAST_HASH_WORDS (DWMAC_MULTICAST_FILTER_BINS / 32)

static void dwmac_mcast_hash_cb(struct net_if *iface, const struct net_eth_mcast_addr *mcast_addr,
				void *user_data)
{
	uint32_t *hash_table = user_data;
	uint32_t crc;
	uint32_t index;

	ARG_UNUSED(iface);

	crc = sys_bit_rev32(crc32_ieee(mcast_addr->addr.addr, sizeof(mcast_addr->addr.addr)));
	index = crc >> (32 - LOG2(DWMAC_MULTICAST_FILTER_BINS));

	hash_table[index / 32] |= BIT(index % 32);
}

void dwmac_setup_multicast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct dwmac_priv *p = dev->data;
	uint32_t hash_table[DWMAC_MCAST_HASH_WORDS] = {0};

	ARG_UNUSED(filter);

	net_eth_mcast_addr_foreach(p->iface, dwmac_mcast_hash_cb, hash_table);

	for (unsigned int i = 0; i < DWMAC_MCAST_HASH_WORDS; i++) {
		DWMAC_REG_WRITE(DWMAC_MCAST_HASH_TABLE_REG(i), hash_table[i]);
	}
}
