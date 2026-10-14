/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the Xilinx Zynq-7000 and ZynqMP processing systems.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/device_mmio.h>

#include "eth_cdns_macb_priv.h"

/* The descriptor rings are not cache maintained, they need an uncached region */
BUILD_ASSERT(IS_ENABLED(CONFIG_NOCACHE_MEMORY), "CONFIG_NOCACHE_MEMORY is required");

/*
 * The DMA bus master interface of the GEM is a
 * - 32-bit AHB interface on Zynq-7000
 * - 64-bit AXI interface on ZynqMP
 */
#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_gem)
#define DATA_BUS_WIDTH 64
#else
#define DATA_BUS_WIDTH 32
#endif

CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

struct eth_xlnx_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_xlnx_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	/* The rings live in the nocache section, which the MMU maps one to one */
	p->rings_phys = COND_CODE_1(CONFIG_MMU, (k_mem_phys_addr(cfg->macb.rings)),
				    (POINTER_TO_UINT(cfg->macb.rings)));

	return cfg->platform_init(dev);
}

/* The clocks come from the SoC clock controller, the clock ID is the subsystem */
#define ETH_XLNX_CLOCK(n, name)                                                                    \
	CDNS_MACB_DT_INST_CLOCK(                                                                   \
		n, name,                                                                           \
		(clock_control_subsys_t)(uintptr_t)DT_INST_PHA_BY_NAME_OR(n, clocks, name, id, 0))

#define ETH_XLNX_CDNS_MACB_DEVICE(n, prefix, caps_)                                                \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
                                                                                                   \
	CDNS_MACB_DT_INST_PINCTRL_DEFINE(n)                                                        \
                                                                                                   \
	static struct cdns_macb_rings prefix##n##_rings __nocache_noinit                           \
		__aligned(CDNS_MACB_RINGS_ALIGN);                                                  \
                                                                                                   \
	static int prefix##n##_platform_init(const struct device *dev)                             \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* set up the IRQ (still masked for now), index 1 is the wake-up IRQ */            \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_macb_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_xlnx_cdns_macb_config prefix##n##_config = {                       \
		.macb = {                                                                          \
			DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                      \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                             \
			CDNS_MACB_DT_INST_PINCTRL_INIT(n)                                          \
			CDNS_MACB_DT_INST_RESET_INIT(n)                                            \
			.rings = &prefix##n##_rings,                                               \
			.clks = {                                                                  \
				[CDNS_MACB_CLK_PCLK] = ETH_XLNX_CLOCK(n, pclk),                    \
				[CDNS_MACB_CLK_HCLK] = ETH_XLNX_CLOCK(n, hclk),                    \
				[CDNS_MACB_CLK_TX] = ETH_XLNX_CLOCK(n, tx_clk),                    \
				[CDNS_MACB_CLK_RX] = ETH_XLNX_CLOCK(n, rx_clk),                    \
				[CDNS_MACB_CLK_TSU] = ETH_XLNX_CLOCK(n, tsu_clk),                  \
			},                                                                         \
			.caps = (caps_),                                                           \
			.dma_burst_length = 16,                                                    \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
		},                                                                                 \
		.platform_init = prefix##n##_platform_init,                                        \
	};                                                                                         \
                                                                                                   \
	static struct cdns_macb_priv prefix##n##_data;                                             \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, cdns_macb_probe, NULL, &prefix##n##_data,                 \
				      &prefix##n##_config, CONFIG_ETH_INIT_PRIORITY,               \
				      &cdns_macb_api, NET_ETH_MTU);

/*
 * Zynq-7000: the DMA can stop under heavy load after a used bit read (Zynq
 * TRM 16.7.4), and the MAC does not support half duplex at 1000 Mbit/s.
 */
#define ETH_ZYNQ_GEM_CAPS                                                                          \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_NEEDS_RSTONUBR |                   \
	 CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_ZYNQ_GEM_DEVICE(n) ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynq, ETH_ZYNQ_GEM_CAPS)

#define DT_DRV_COMPAT xlnx_zynq_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQ_GEM_DEVICE)
#undef DT_DRV_COMPAT

/* ZynqMP: the DMA prefetches descriptors */
#define ETH_ZYNQMP_GEM_CAPS                                                                        \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_BD_RD_PREFETCH |                   \
	 CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_ZYNQMP_GEM_DEVICE(n) ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynqmp, ETH_ZYNQMP_GEM_CAPS)

#define DT_DRV_COMPAT xlnx_zynqmp_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQMP_GEM_DEVICE)
#undef DT_DRV_COMPAT
