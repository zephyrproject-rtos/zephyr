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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

#include "eth_cdns_macb_priv.h"

/* The DMA bus master interface of the GEM is a 32-bit interface on the FU540 */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(32);

/* Required by the named DEVICE_MMIO macros, the TX clock device owns the register */
#define DEV_CFG(dev)  ((const struct eth_sifive_tx_clk_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_sifive_tx_clk_data *)(dev)->data)

/*
 * The GEMGXL management register selects the GEM TX clock: 0 for the 125 MHz
 * gigabit clock, 1 for the 25 MHz / 2.5 MHz clock of the slower speeds.
 */
#define FU540_GEMGXL_MGMT_TX_CLK_SLOW BIT(0)

struct eth_sifive_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

/*
 * TX clock
 *
 * The management register is a clock of its own, the transmit clock of the
 * MAC, as in the Linux driver.
 */

struct eth_sifive_tx_clk_config {
	DEVICE_MMIO_NAMED_ROM(mgmt);
};

struct eth_sifive_tx_clk_data {
	DEVICE_MMIO_NAMED_RAM(mgmt);
	uint32_t rate;
};

static int eth_sifive_tx_clk_on_off(__unused const struct device *dev,
				    __unused clock_control_subsys_t sys)
{
	return 0;
}

static int eth_sifive_tx_clk_get_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				      uint32_t *rate)
{
	struct eth_sifive_tx_clk_data *data = dev->data;

	*rate = data->rate;

	return 0;
}

static int eth_sifive_tx_clk_set_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				      clock_control_subsys_rate_t rate)
{
	struct eth_sifive_tx_clk_data *data = dev->data;
	uint32_t rate_hz = (uint32_t)(uintptr_t)rate;

	if ((rate_hz != MHZ(125)) && (rate_hz != MHZ(25)) && (rate_hz != KHZ(2500))) {
		return -EINVAL;
	}

	sys_write32((rate_hz == MHZ(125)) ? 0U : FU540_GEMGXL_MGMT_TX_CLK_SLOW,
		    DEVICE_MMIO_NAMED_GET(dev, mgmt));
	data->rate = rate_hz;

	return 0;
}

static int eth_sifive_tx_clk_init(const struct device *dev)
{
	struct eth_sifive_tx_clk_data *data = dev->data;

	DEVICE_MMIO_NAMED_MAP(dev, mgmt, K_MEM_CACHE_NONE);

	if ((sys_read32(DEVICE_MMIO_NAMED_GET(dev, mgmt)) & FU540_GEMGXL_MGMT_TX_CLK_SLOW) != 0U) {
		data->rate = MHZ(25);
	} else {
		data->rate = MHZ(125);
	}

	return 0;
}

static DEVICE_API(clock_control, eth_sifive_tx_clk_api) = {
	.on = eth_sifive_tx_clk_on_off,
	.off = eth_sifive_tx_clk_on_off,
	.get_rate = eth_sifive_tx_clk_get_rate,
	.set_rate = eth_sifive_tx_clk_set_rate,
};

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_sifive_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

/* The GEM DMA of the FU540 is coherent with the caches of the cores */
#define ETH_SIFIVE_CDNS_MACB_CAPS                                                                  \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_SIFIVE_CDNS_MACB_DEVICE(n)                                                             \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
                                                                                                   \
	static const struct eth_sifive_tx_clk_config eth##n##_tx_clk_config = {                    \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mgmt, DT_DRV_INST(n)),                          \
	};                                                                                         \
                                                                                                   \
	static struct eth_sifive_tx_clk_data eth##n##_tx_clk_data;                                 \
                                                                                                   \
	DEVICE_DEFINE(eth##n##_tx_clk, DEVICE_DT_NAME(DT_DRV_INST(n)) "_tx_clk",                   \
		      eth_sifive_tx_clk_init, NULL, &eth##n##_tx_clk_data,                         \
		      &eth##n##_tx_clk_config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,   \
		      &eth_sifive_tx_clk_api);                                                     \
                                                                                                   \
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
			.clks = {                                                                  \
				[CDNS_MACB_CLK_PCLK] = CDNS_MACB_DT_INST_CLOCK(n, pclk, NULL),     \
				[CDNS_MACB_CLK_HCLK] = CDNS_MACB_DT_INST_CLOCK(n, hclk, NULL),     \
				[CDNS_MACB_CLK_TX] = {.dev = DEVICE_GET(eth##n##_tx_clk)},         \
				[CDNS_MACB_CLK_TSU] = CDNS_MACB_DT_INST_CLOCK(n, tsu_clk, NULL),   \
			},                                                                         \
			.caps = ETH_SIFIVE_CDNS_MACB_CAPS,                                         \
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

DT_INST_FOREACH_STATUS_OKAY(ETH_SIFIVE_CDNS_MACB_DEVICE)
