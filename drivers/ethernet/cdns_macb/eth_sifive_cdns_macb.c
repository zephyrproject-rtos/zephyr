/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the SiFive FU540.
 */

#define DT_DRV_COMPAT sifive_fu540_c000_gem

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

#include "eth_cdns_macb_priv.h"

/* The DMA bus master interface of the GEM is a 32-bit interface on the FU540 */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(32);

/* Required by the named DEVICE_MMIO macros */
#define DEV_CFG(dev)  ((const struct eth_sifive_cdns_macb_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_sifive_cdns_macb_data *)(dev)->data)

/*
 * The GEMGXL management register selects the GEM TX clock: 0 for the 125 MHz
 * gigabit clock, 1 for the 25 MHz / 2.5 MHz clock of the slower speeds.
 */
#define FU540_GEMGXL_MGMT_TX_CLK_SLOW BIT(0)

struct eth_sifive_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	DEVICE_MMIO_NAMED_ROM(mgmt);
	int (*platform_init)(const struct device *dev);
};

struct eth_sifive_cdns_macb_data {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_priv macb;
	DEVICE_MMIO_NAMED_RAM(mgmt);
};

void cdns_macb_platform_link_speed_changed(const struct device *dev, enum phy_link_speed speed)
{
	sys_write32(PHY_LINK_IS_SPEED_1000M(speed) ? 0U : FU540_GEMGXL_MGMT_TX_CLK_SLOW,
		    DEVICE_MMIO_NAMED_GET(dev, mgmt));
}

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_sifive_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	DEVICE_MMIO_NAMED_MAP(dev, mgmt, K_MEM_CACHE_NONE);

	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

/* The GEM DMA of the FU540 is coherent with the caches of the cores */
#define ETH_SIFIVE_CDNS_MACB_CAPS                                                                  \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_SIFIVE_CDNS_MACB_DEVICE(n)                                                             \
	static struct cdns_macb_rings eth##n##_rings __aligned(CDNS_MACB_RINGS_ALIGN);             \
                                                                                                   \
	static int eth##n##_platform_init(const struct device *dev)                                \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* set up the IRQ (still masked for now) */                                        \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_macb_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_sifive_cdns_macb_config eth##n##_config = {                        \
		.macb = {                                                                          \
			DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                      \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                             \
			IF_ENABLED(CONFIG_PTP_CLOCK_CDNS_MACB,                                     \
				   (.ptp_clock = DEVICE_DT_GET_OR_NULL(                            \
						   DT_INST_CHILD(n, ptp_clock)),))                 \
			.rings = &eth##n##_rings,                                                  \
			.caps = ETH_SIFIVE_CDNS_MACB_CAPS,                                         \
			.dma_burst_length = 16,                                                    \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
		},                                                                                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mgmt, DT_DRV_INST(n)),                          \
		.platform_init = eth##n##_platform_init,                                           \
	};                                                                                         \
                                                                                                   \
	static struct eth_sifive_cdns_macb_data eth##n##_data;                                     \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, cdns_macb_probe, NULL, &eth##n##_data, &eth##n##_config,  \
				      CONFIG_ETH_INIT_PRIORITY, &cdns_macb_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_SIFIVE_CDNS_MACB_DEVICE)
