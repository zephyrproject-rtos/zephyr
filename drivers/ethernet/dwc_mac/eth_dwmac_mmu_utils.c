/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME dwmac_plat
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr/kernel/mm.h>
#include <zephyr/cache.h>
#include "eth_dwmac_priv.h"

void dwmac_map_descriptors(const struct device *dev, struct dwmac_dma_desc *rx_desc,
			  int rx_desc_count, struct dwmac_dma_desc *tx_desc,
			  int tx_desc_count)
{
	struct dwmac_priv *p = dev->data;
	uint8_t *tx_desc_uncached_addr, *rx_desc_uncached_addr;
	uintptr_t tx_desc_phys_addr, rx_desc_phys_addr;

	/* make sure no valid cache lines map to the descriptor area */
	sys_cache_data_invd_range(rx_desc,
				  sizeof(struct dwmac_dma_desc) * rx_desc_count);
	sys_cache_data_invd_range(tx_desc,
				  sizeof(struct dwmac_dma_desc) * tx_desc_count);

	rx_desc_phys_addr = k_mem_phys_addr(rx_desc);
	tx_desc_phys_addr = k_mem_phys_addr(tx_desc);

	/* remap descriptor rings uncached */
	k_mem_map_phys_bare(&rx_desc_uncached_addr, rx_desc_phys_addr,
			    sizeof(struct dwmac_dma_desc) * rx_desc_count,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);
	k_mem_map_phys_bare(&tx_desc_uncached_addr, tx_desc_phys_addr,
			    sizeof(struct dwmac_dma_desc) * tx_desc_count,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);

	LOG_DBG("rx desc virt %p uncached %p phys 0x%lx",
		rx_desc, rx_desc_uncached_addr, rx_desc_phys_addr);
	LOG_DBG("tx desc virt %p uncached %p phys 0x%lx",
		tx_desc, tx_desc_uncached_addr, tx_desc_phys_addr);

	p->tx_descs = (void *)tx_desc_uncached_addr;
	p->rx_descs = (void *)rx_desc_uncached_addr;

	p->tx_descs_phys = UINT_TO_POINTER(tx_desc_phys_addr);
	p->rx_descs_phys = UINT_TO_POINTER(rx_desc_phys_addr);
}
