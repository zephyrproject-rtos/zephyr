/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define LOG_MODULE_NAME dwmac_plat
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT snps_designware_ethernet

#include <sys/types.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/zephyr.h>
#include <zephyr/cache.h>
#include <zephyr/net/ethernet.h>

#include "eth_dwmac_priv.h"

int dwmac_bus_init(struct dwmac_priv *p)
{
	p->base_addr = DT_INST_REG_ADDR(0);
	return 0;
}

#if (CONFIG_DCACHE_LINE_SIZE+0 == 0)
#error "CONFIG_DCACHE_LINE_SIZE must be configured to a non-zero value"
#endif

static struct dwmac_dma_desc __aligned(CONFIG_DCACHE_LINE_SIZE)
			dwmac_tx_rx_descriptors[NB_TX_DESCS + NB_RX_DESCS];

static const uint8_t dwmac_mac_addr[6] = DT_INST_PROP(0, local_mac_address);

void dwmac_platform_init(struct dwmac_priv *p)
{
	uint8_t *desc_uncached_addr;
	uintptr_t desc_phys_addr;

	/* make sure no valid cache lines map to the descriptor area */
	sys_cache_data_range(dwmac_tx_rx_descriptors,
			     sizeof(dwmac_tx_rx_descriptors),
			     K_CACHE_INVD);

	desc_phys_addr = z_mem_phys_addr(dwmac_tx_rx_descriptors);

	/* remap descriptor rings uncached */
	z_phys_map(&desc_uncached_addr, desc_phys_addr,
		   sizeof(dwmac_tx_rx_descriptors),
		   K_MEM_PERM_RW | K_MEM_CACHE_NONE);

	LOG_DBG("desc virt %p uncached %p phys 0x%lx",
		dwmac_tx_rx_descriptors, desc_uncached_addr, desc_phys_addr);

	p->tx_descs = (void *)desc_uncached_addr;
	desc_uncached_addr += NB_TX_DESCS * sizeof(struct dwmac_dma_desc);
	p->rx_descs = (void *)desc_uncached_addr;

	p->tx_descs_phys = desc_phys_addr;
	desc_phys_addr += NB_TX_DESCS * sizeof(struct dwmac_dma_desc);
	p->rx_descs_phys = desc_phys_addr;

	/* basic configuration for this platform */
	REG_WRITE(MAC_CONF,
		  MAC_CONF_PS |
		  MAC_CONF_FES |
		  MAC_CONF_DM);
	REG_WRITE(DMA_SYSBUS_MODE,
		  DMA_SYSBUS_MODE_AAL |
#ifdef CONFIG_64BIT
		  DMA_SYSBUS_MODE_EAME |
#endif
		  DMA_SYSBUS_MODE_FB);

	/* set up IRQs (still masked for now) */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* retrieve MAC address */
	memcpy(p->mac_addr, dwmac_mac_addr, sizeof(p->mac_addr));
}

/* Our private device instance */
static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0,
			      dwmac_probe,
			      NULL,
			      &dwmac_instance,
			      NULL,
			      CONFIG_ETH_INIT_PRIORITY,
			      &dwmac_api,
			      NET_ETH_MTU);
