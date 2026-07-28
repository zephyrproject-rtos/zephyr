/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dwmac_core, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/bit_rev.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/device_mmio.h>

#include "eth_dwmac_priv.h"

#ifdef CONFIG_ETH_DWC_ETHER_1000_CORE
#define DWMAC_MULTICAST_FILTER_HASH_TABLE_HIGH DWMAC_MACHTHR
#define DWMAC_MULTICAST_FILTER_HASH_TABLE_LOW  DWMAC_MACHTLR
#endif
#ifdef CONFIG_ETH_DWC_ETHER_QOS_CORE
#define DWMAC_MULTICAST_FILTER_HASH_TABLE_HIGH MAC_HASH_TABLE(1)
#define DWMAC_MULTICAST_FILTER_HASH_TABLE_LOW  MAC_HASH_TABLE(0)
#endif

void dwmac_setup_multicast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct dwmac_priv *p = dev->data;
	uint32_t crc;
	uint32_t hash_table[2];
	uint32_t hash_index;

	crc = sys_bit_rev32(crc32_ieee(filter->mac_address.addr, sizeof(struct net_eth_addr)));
	hash_index = (crc >> 26) & 0x3f;

	__ASSERT_NO_MSG(hash_index < ARRAY_SIZE(p->hash_index_cnt));

	hash_table[0] = DWMAC_REG_READ(DWMAC_MULTICAST_FILTER_HASH_TABLE_LOW);
	hash_table[1] = DWMAC_REG_READ(DWMAC_MULTICAST_FILTER_HASH_TABLE_HIGH);

	if (filter->set) {
		p->hash_index_cnt[hash_index]++;
		hash_table[hash_index / 32] |= (1 << (hash_index % 32));
	} else {
		if (p->hash_index_cnt[hash_index] == 0) {
			return; /* No hash at index to remove */
		}

		p->hash_index_cnt[hash_index]--;
		if (p->hash_index_cnt[hash_index] == 0) {
			hash_table[hash_index / 32] &= ~(1 << (hash_index % 32));
		}
	}

	DWMAC_REG_WRITE(DWMAC_MULTICAST_FILTER_HASH_TABLE_LOW, hash_table[0]);
	DWMAC_REG_WRITE(DWMAC_MULTICAST_FILTER_HASH_TABLE_HIGH, hash_table[1]);
}
