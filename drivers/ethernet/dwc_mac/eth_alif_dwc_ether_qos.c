/*
 * Driver for Synopsys DesignWare MAC
 *
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Alif Ensemble specific glue.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT alif_ensemble_eth

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include "eth_dwmac_priv.h"

/* The DMA bus master interface is a 64-bit AXI interface on this IP */
#define DATA_BUS_WIDTH 64

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii),
	     "The Ethernet MAC only supports RMII");

/*
 * DMA to the TCM of the M55 cores is not supported. The system bus reaches a
 * TCM only at its external address, while the core driver hands the MAC the
 * local addresses untranslated. The TCMs also come up secure-only, so the MAC
 * would need a non-secure window in them, with matching TGU and SAU
 * programming. Neither is implemented. Packet buffers and DMA descriptors
 * live in the zephyr,sram region, so that has to be a system SRAM.
 */
BUILD_ASSERT(!DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_sram), arm_dtcm),
	     "Ethernet DMA to DTCM is not supported, choose a system SRAM as zephyr,sram");

/*
 * Address-aligned, undefined-length INCR bursts; with BLEN32 to BLEN256 clear,
 * a burst is at most 16 beats. Four outstanding reads, but only two outstanding
 * writes, as the DWC EQoS databook recommends: when a received frame ends early,
 * the master still completes the write bursts already issued, with dummy beats.
 */
#define ALIF_ETH_DMA_SYSBUS_MODE                                                                   \
	(DMA_SYSBUS_MODE_AAL | FIELD_PREP(DMA_SYSBUS_MODE_RD_OSR_LMT, 3) |                         \
	 FIELD_PREP(DMA_SYSBUS_MODE_WR_OSR_LMT, 1))

#define ALIF_ETH_CLOCK_SUBSYS(name)                                                                \
	((clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(0, name, clkid))

PINCTRL_DT_INST_DEFINE(0);

/* The RMII clock source has to be selected before its gate is opened */
static const clock_control_subsys_t eth_clocks[] = {
	ALIF_ETH_CLOCK_SUBSYS(rmii_src),
	ALIF_ETH_CLOCK_SUBSYS(rmii),
};

int dwmac_bus_init(const struct device *dev)
{
	const struct dwmac_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->clock)) {
		LOG_ERR("Clock controller not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(PINCTRL_DT_INST_DEV_CONFIG_GET(0), PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins (%d)", ret);
		return ret;
	}

	for (size_t n = 0; n < ARRAY_SIZE(eth_clocks); n++) {
		ret = clock_control_on(cfg->clock, eth_clocks[n]);
		if (ret < 0) {
			LOG_ERR("Failed to enable ethernet clock #%zu (%d)", n, ret);
			return ret;
		}
	}

	return 0;
}

#define DESCRIPTOR_ALIGNMENT ((DATA_BUS_WIDTH) / (BITS_PER_BYTE))
#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache __aligned(DESCRIPTOR_ALIGNMENT)
#else
/*
 * The core driver maintains the cache for the packet buffers but not for the
 * descriptors, so those have to live in memory the DMA and the CPU see alike.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_DCACHE),
	     "DMA descriptors would be cached, CONFIG_NOCACHE_MEMORY is needed");
#define __desc_mem __aligned(DESCRIPTOR_ALIGNMENT)
#endif

/* Descriptor rings in uncached memory */
static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

int dwmac_platform_init(const struct device *dev)
{
	const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0);
	struct dwmac_priv *p = dev->data;
	int ret;

	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	/* basic configuration for this platform */
	DWMAC_REG_WRITE(MAC_CONF, MAC_CONF_PS | MAC_CONF_FES | MAC_CONF_DM);
	DWMAC_REG_WRITE(DMA_SYSBUS_MODE, ALIF_ETH_DMA_SYSBUS_MODE);

	/* set up the IRQ; its sources stay masked in the MAC until the interface is initialized */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	ret = net_eth_mac_load(&mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		LOG_WRN("No MAC address configured");
		return 0;
	}

	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
	}

	return ret;
}

static const struct dwmac_config dwmac_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.phy_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(0, phy_handle)),
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.mac_clk = ALIF_ETH_CLOCK_SUBSYS(csr),
};

static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0, dwmac_probe, NULL, &dwmac_instance, &dwmac_config,
			      CONFIG_ETH_INIT_PRIORITY, &dwmac_api, NET_ETH_MTU);
