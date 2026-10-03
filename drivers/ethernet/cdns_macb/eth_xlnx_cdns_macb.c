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
#include <zephyr/sys/sys_io.h>

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

/* Required by the named DEVICE_MMIO macros, the TX clock device owns the register */
#define DEV_CFG(dev)  ((const struct eth_xlnx_tx_clk_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_xlnx_tx_clk_data *)(dev)->data)

struct eth_xlnx_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

/*
 * TX clock
 *
 * The GEM TX clock is derived from a PLL through two 6-bit dividers in the
 * SoC clock controller. The register is a clock of its own here, the transmit
 * clock of the MAC. Its layout differs between the SoCs.
 *
 * Zynq-7000, SLCR GEMx_CLK_CTRL:
 *   [25..20] DIVISOR1, [13..8] DIVISOR0, [0] CLKACT
 *
 * ZynqMP, CRL_APB GEMx_REF_CTRL:
 *   [26] RX_CLKACT, [25] CLKACT, [21..16] DIVISOR1, [13..8] DIVISOR0
 *   The CRL_APB registers are write protected by CRL_WPROT.
 */
#define XLNX_GEM_CLK_DIV_MAX 63U

#define XLNX_GEM_CLK_CTRL_DIV0 GENMASK(13, 8)

#define ZYNQ_SLCR_GEM_CLK_CTRL_DIV1 GENMASK(25, 20)

#define ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV1      GENMASK(21, 16)
#define ZYNQMP_CRL_APB_GEM_REF_CTRL_CLKACT    BIT(25)
#define ZYNQMP_CRL_APB_GEM_REF_CTRL_RX_CLKACT BIT(26)
#define ZYNQMP_CRL_APB_WPROT                  0xFF5E001CUL
#define ZYNQMP_CRL_APB_WPROT_ACTIVE           BIT(0)

struct eth_xlnx_tx_clk_config {
	DEVICE_MMIO_NAMED_ROM(clkc);
	/* The PLL feeding the dividers */
	struct cdns_macb_clock pll;
	uint32_t div1_mask;
	/* Clock enables to set along with the dividers */
	uint32_t enable;
	/* The register is write protected by CRL_WPROT */
	bool wprot;
};

struct eth_xlnx_tx_clk_data {
	DEVICE_MMIO_NAMED_RAM(clkc);
};

static int eth_xlnx_tx_clk_on_off(__unused const struct device *dev,
				  __unused clock_control_subsys_t sys)
{
	return 0;
}

static int eth_xlnx_tx_clk_get_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				    uint32_t *rate)
{
	const struct eth_xlnx_tx_clk_config *cfg = dev->config;
	uint32_t val = sys_read32(DEVICE_MMIO_NAMED_GET(dev, clkc));
	uint32_t div0 = FIELD_GET(XLNX_GEM_CLK_CTRL_DIV0, val);
	uint32_t div1 = FIELD_GET(cfg->div1_mask, val);
	uint32_t pll_rate;
	int ret;

	if ((div0 == 0U) || (div1 == 0U)) {
		return -EIO;
	}

	ret = clock_control_get_rate(cfg->pll.dev, cfg->pll.subsys, &pll_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = pll_rate / (div0 * div1);

	return 0;
}

static int eth_xlnx_tx_clk_set_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				    clock_control_subsys_rate_t rate)
{
	const struct eth_xlnx_tx_clk_config *cfg = dev->config;
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, clkc);
	uint32_t target = (uint32_t)(uintptr_t)rate;
	uint32_t best_err = UINT32_MAX, best_div0 = 0U, best_div1 = 0U;
	uint32_t pll_rate, val, wprot = 0U;
	int ret;

	if (target == 0U) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->pll.dev, cfg->pll.subsys, &pll_rate);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t div0 = 1U; (div0 <= XLNX_GEM_CLK_DIV_MAX) && (best_err > 0U); div0++) {
		for (uint32_t div1 = 1U; div1 <= XLNX_GEM_CLK_DIV_MAX; div1++) {
			uint32_t div_rate = pll_rate / (div0 * div1);
			uint32_t err = (div_rate > target) ? (div_rate - target)
							   : (target - div_rate);

			if (err < best_err) {
				best_err = err;
				best_div0 = div0;
				best_div1 = div1;
			}
			if (err == 0U) {
				break;
			}
		}
	}

	LOG_DBG("TX clock dividers %u/%u for %u Hz from %u Hz", best_div0, best_div1, target,
		pll_rate);

	val = sys_read32(reg);
	val &= ~(XLNX_GEM_CLK_CTRL_DIV0 | cfg->div1_mask);
	val |= FIELD_PREP(XLNX_GEM_CLK_CTRL_DIV0, best_div0) |
	       FIELD_PREP(cfg->div1_mask, best_div1) | cfg->enable;

	/* Lift the write protection for the write and restore it */
	if (cfg->wprot) {
		wprot = sys_read32(ZYNQMP_CRL_APB_WPROT);
		if ((wprot & ZYNQMP_CRL_APB_WPROT_ACTIVE) != 0U) {
			sys_write32(wprot & ~ZYNQMP_CRL_APB_WPROT_ACTIVE, ZYNQMP_CRL_APB_WPROT);
		}
	}
	sys_write32(val, reg);
	if ((wprot & ZYNQMP_CRL_APB_WPROT_ACTIVE) != 0U) {
		sys_write32(wprot, ZYNQMP_CRL_APB_WPROT);
	}

	return 0;
}

static int eth_xlnx_tx_clk_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, clkc, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, eth_xlnx_tx_clk_api) = {
	.on = eth_xlnx_tx_clk_on_off,
	.off = eth_xlnx_tx_clk_on_off,
	.get_rate = eth_xlnx_tx_clk_get_rate,
	.set_rate = eth_xlnx_tx_clk_set_rate,
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

#define ETH_XLNX_CDNS_MACB_DEVICE(n, prefix, caps_, div1_mask_, enable_, wprot_)                   \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pll), "the pll clock is required");                \
                                                                                                   \
	static const struct eth_xlnx_tx_clk_config prefix##n##_tx_clk_config = {                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(clkc, DT_DRV_INST(n)),                          \
		.pll = CDNS_MACB_DT_INST_CLOCK(n, pll, NULL),                                      \
		.div1_mask = (div1_mask_),                                                         \
		.enable = (enable_),                                                               \
		.wprot = (wprot_),                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct eth_xlnx_tx_clk_data prefix##n##_tx_clk_data;                                \
                                                                                                   \
	DEVICE_DEFINE(prefix##n##_tx_clk, DEVICE_DT_NAME(DT_DRV_INST(n)) "_tx_clk",                \
		      eth_xlnx_tx_clk_init, NULL, &prefix##n##_tx_clk_data,                        \
		      &prefix##n##_tx_clk_config, PRE_KERNEL_1,                                    \
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &eth_xlnx_tx_clk_api);                   \
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
			IF_ENABLED(CONFIG_PTP_CLOCK_CDNS_MACB,                                     \
				   (.ptp_clock = DEVICE_DT_GET_OR_NULL(                            \
						   DT_INST_CHILD(n, ptp_clock)),))                 \
			CDNS_MACB_DT_INST_PINCTRL_INIT(n)                                          \
			.rings = &prefix##n##_rings,                                               \
			.clks = {                                                                  \
				[CDNS_MACB_CLK_PCLK] = CDNS_MACB_DT_INST_CLOCK(n, pclk, NULL),     \
				[CDNS_MACB_CLK_HCLK] = CDNS_MACB_DT_INST_CLOCK(n, hclk, NULL),     \
				[CDNS_MACB_CLK_TX] = {.dev = DEVICE_GET(prefix##n##_tx_clk)},      \
				[CDNS_MACB_CLK_RX] = CDNS_MACB_DT_INST_CLOCK(n, rx_clk, NULL),     \
				[CDNS_MACB_CLK_TSU] = CDNS_MACB_DT_INST_CLOCK(n, tsu_clk, NULL),   \
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

#define ETH_ZYNQ_GEM_DEVICE(n)                                                                     \
	ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynq, ETH_ZYNQ_GEM_CAPS, ZYNQ_SLCR_GEM_CLK_CTRL_DIV1, 0U, \
				  false)

#define DT_DRV_COMPAT xlnx_zynq_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQ_GEM_DEVICE)
#undef DT_DRV_COMPAT

/* ZynqMP: the DMA prefetches descriptors */
#define ETH_ZYNQMP_GEM_CAPS                                                                        \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_BD_RD_PREFETCH |                   \
	 CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_ZYNQMP_GEM_DEVICE(n)                                                                   \
	ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynqmp, ETH_ZYNQMP_GEM_CAPS,                              \
				  ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV1,                                \
				  ZYNQMP_CRL_APB_GEM_REF_CTRL_CLKACT |                             \
					  ZYNQMP_CRL_APB_GEM_REF_CTRL_RX_CLKACT,                   \
				  true)

#define DT_DRV_COMPAT xlnx_zynqmp_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQMP_GEM_DEVICE)
#undef DT_DRV_COMPAT
