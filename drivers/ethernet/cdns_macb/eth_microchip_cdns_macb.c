/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the Microchip PolarFire SoC and PIC64GX.
 */

#define DT_DRV_COMPAT microchip_mpfs_gem

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>

#include "eth_cdns_macb_priv.h"

/* The DMA bus master interface of the GEM is a 64-bit AXI interface */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(64);

struct eth_microchip_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_microchip_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

/* The clocks are fixed clocks, devices of their own */
#define ETH_MICROCHIP_CLOCK(n, name) CDNS_MACB_DT_INST_CLOCK(n, name, NULL)

#define ETH_MICROCHIP_CDNS_MACB_CAPS CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE

/*
 * The GEM DMA reaches the cached DDR window through the front port of the core
 * complex, which keeps it coherent with the caches of the cores. The descriptor
 * rings therefore live in ordinary memory.
 */

#define ETH_MICROCHIP_CDNS_MACB_DEVICE(n)                                                          \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
                                                                                                   \
	CDNS_MACB_DT_INST_PINCTRL_DEFINE(n)                                                        \
                                                                                                   \
	static struct cdns_macb_rings eth##n##_rings __aligned(CDNS_MACB_RINGS_ALIGN);             \
                                                                                                   \
	static int eth##n##_platform_init(const struct device *dev)                                \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* set up the IRQ of queue 0 (still masked for now) */                             \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_macb_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_microchip_cdns_macb_config eth##n##_config = {                     \
		.macb = {                                                                          \
			DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                      \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                             \
			IF_ENABLED(CONFIG_PTP_CLOCK_CDNS_MACB,                                     \
				   (.ptp_clock = DEVICE_DT_GET_OR_NULL(                            \
						   DT_INST_CHILD(n, ptp_clock)),))                 \
			CDNS_MACB_DT_INST_PINCTRL_INIT(n)                                          \
			CDNS_MACB_DT_INST_RESET_INIT(n)                                            \
			.rings = &eth##n##_rings,                                                  \
			.clks = {                                                                  \
				[CDNS_MACB_CLK_PCLK] = ETH_MICROCHIP_CLOCK(n, pclk),               \
				[CDNS_MACB_CLK_HCLK] = ETH_MICROCHIP_CLOCK(n, hclk),               \
				[CDNS_MACB_CLK_TX] = ETH_MICROCHIP_CLOCK(n, tx_clk),               \
				[CDNS_MACB_CLK_TSU] = ETH_MICROCHIP_CLOCK(n, tsu_clk),             \
			},                                                                         \
			.caps = ETH_MICROCHIP_CDNS_MACB_CAPS,                                      \
			.dma_burst_length = 16,                                                    \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
		},                                                                                 \
		.platform_init = eth##n##_platform_init,                                           \
	};                                                                                         \
                                                                                                   \
	static struct cdns_macb_priv eth##n##_data;                                                \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, cdns_macb_probe, NULL, &eth##n##_data, &eth##n##_config,  \
				      CONFIG_ETH_INIT_PRIORITY, &cdns_macb_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_MICROCHIP_CDNS_MACB_DEVICE)
