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

/* Required by the named DEVICE_MMIO macros */
#define DEV_CFG(dev)  ((const struct eth_xlnx_cdns_macb_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_xlnx_cdns_macb_data *)(dev)->data)

/*
 * TX clock configuration
 *
 * The GEM TX clock is derived from a PLL through two 6-bit dividers in the
 * SoC clock controller. The register layout differs between the SoCs.
 *
 * Zynq-7000, SLCR GEMx_CLK_CTRL:
 *   [25..20] DIVISOR1, [13..8] DIVISOR0, [0] CLKACT
 *
 * ZynqMP, CRL_APB GEMx_REF_CTRL:
 *   [26] RX_CLKACT, [25] CLKACT, [21..16] DIVISOR1, [13..8] DIVISOR0
 *   The CRL_APB registers are write protected by CRL_WPROT.
 */
#define XLNX_GEM_CLK_DIV_MAX 63U

#define ZYNQ_SLCR_GEM_CLK_CTRL_DIV0 GENMASK(13, 8)
#define ZYNQ_SLCR_GEM_CLK_CTRL_DIV1 GENMASK(25, 20)

#define ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV0     GENMASK(13, 8)
#define ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV1     GENMASK(21, 16)
#define ZYNQMP_CRL_APB_GEM_REF_CTRL_CLKACT   BIT(25)
#define ZYNQMP_CRL_APB_GEM_REF_CTRL_RX_CLKACT BIT(26)
#define ZYNQMP_CRL_APB_WPROT                 0xFF5E001CUL
#define ZYNQMP_CRL_APB_WPROT_ACTIVE          BIT(0)

/* Frequency error the RGMII interface tolerates, in ppm */
#define XLNX_GEM_TX_CLK_PPM_MAX 50U

struct eth_xlnx_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	DEVICE_MMIO_NAMED_ROM(clkc);
	/* Rate of the PLL feeding the TX clock dividers */
	uint32_t pll_clock_frequency;
	int (*platform_init)(const struct device *dev);
	void (*set_tx_clk_dividers)(const struct device *dev, uint32_t div0, uint32_t div1);
};

struct eth_xlnx_cdns_macb_data {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_priv macb;
	DEVICE_MMIO_NAMED_RAM(clkc);
};

#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynq_gem)
static void zynq_set_tx_clk_dividers(const struct device *dev, uint32_t div0, uint32_t div1)
{
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, clkc);
	uint32_t val;

	val = sys_read32(reg);
	val &= ~(ZYNQ_SLCR_GEM_CLK_CTRL_DIV0 | ZYNQ_SLCR_GEM_CLK_CTRL_DIV1);
	val |= FIELD_PREP(ZYNQ_SLCR_GEM_CLK_CTRL_DIV0, div0) |
	       FIELD_PREP(ZYNQ_SLCR_GEM_CLK_CTRL_DIV1, div1);
	sys_write32(val, reg);
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynq_gem) */

#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_gem)
static void zynqmp_set_tx_clk_dividers(const struct device *dev, uint32_t div0, uint32_t div1)
{
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, clkc);
	uint32_t val, wprot;

	val = sys_read32(reg);
	val &= ~(ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV0 | ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV1);
	val |= FIELD_PREP(ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV0, div0) |
	       FIELD_PREP(ZYNQMP_CRL_APB_GEM_REF_CTRL_DIV1, div1) |
	       ZYNQMP_CRL_APB_GEM_REF_CTRL_CLKACT | ZYNQMP_CRL_APB_GEM_REF_CTRL_RX_CLKACT;

	/* Lift the CRL_APB write protection for the write and restore it */
	wprot = sys_read32(ZYNQMP_CRL_APB_WPROT);
	if ((wprot & ZYNQMP_CRL_APB_WPROT_ACTIVE) != 0U) {
		sys_write32(wprot & ~ZYNQMP_CRL_APB_WPROT_ACTIVE, ZYNQMP_CRL_APB_WPROT);
	}
	sys_write32(val, reg);
	if ((wprot & ZYNQMP_CRL_APB_WPROT_ACTIVE) != 0U) {
		sys_write32(wprot, ZYNQMP_CRL_APB_WPROT);
	}
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_gem) */

/*
 * Retune the TX clock to the link speed: 125 MHz for 1000 Mbit/s, 25 MHz for
 * 100 Mbit/s and 2.5 MHz for 10 Mbit/s. In MII and RMII mode the PHY is the
 * clock master and nothing needs to be done.
 */
void cdns_macb_platform_link_speed_changed(const struct device *dev, enum phy_link_speed speed)
{
	const struct eth_xlnx_cdns_macb_config *cfg = dev->config;
	uint32_t target, best_err = UINT32_MAX, best_div0 = 0U, best_div1 = 0U;

	if ((cfg->macb.phy_iface == CDNS_MACB_PHY_IFACE_MII) ||
	    (cfg->macb.phy_iface == CDNS_MACB_PHY_IFACE_RMII)) {
		return;
	}

	if (PHY_LINK_IS_SPEED_1000M(speed)) {
		target = MHZ(125);
	} else if (PHY_LINK_IS_SPEED_100M(speed)) {
		target = MHZ(25);
	} else {
		target = KHZ(2500);
	}

	for (uint32_t div0 = 1U; (div0 <= XLNX_GEM_CLK_DIV_MAX) && (best_err > 0U); div0++) {
		for (uint32_t div1 = 1U; div1 <= XLNX_GEM_CLK_DIV_MAX; div1++) {
			uint32_t rate = cfg->pll_clock_frequency / (div0 * div1);
			uint32_t err = (rate > target) ? (rate - target) : (target - rate);

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

	if (((uint64_t)best_err * USEC_PER_SEC / target) > XLNX_GEM_TX_CLK_PPM_MAX) {
		LOG_ERR("unable to generate the %u Hz TX clock from %u Hz (off by %u Hz)", target,
			cfg->pll_clock_frequency, best_err);
	}

	LOG_DBG("TX clock dividers %u/%u for %u Hz", best_div0, best_div1, target);

	cfg->set_tx_clk_dividers(dev, best_div0, best_div1);
}

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_xlnx_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	DEVICE_MMIO_NAMED_MAP(dev, clkc, K_MEM_CACHE_NONE);

	/* The rings live in the nocache section, which the MMU maps one to one */
	p->rings_phys = COND_CODE_1(CONFIG_MMU, (k_mem_phys_addr(cfg->macb.rings)),
				    (POINTER_TO_UINT(cfg->macb.rings)));

	return cfg->platform_init(dev);
}

#define ETH_XLNX_CDNS_MACB_DEVICE(n, prefix, caps_, set_dividers)                                  \
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
			.rings = &prefix##n##_rings,                                               \
			.caps = (caps_),                                                           \
			.dma_burst_length = 16,                                                    \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
		},                                                                                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(clkc, DT_DRV_INST(n)),                          \
		.pll_clock_frequency = DT_INST_PROP(n, clock_frequency),                           \
		.platform_init = prefix##n##_platform_init,                                        \
		.set_tx_clk_dividers = set_dividers,                                               \
	};                                                                                         \
                                                                                                   \
	static struct eth_xlnx_cdns_macb_data prefix##n##_data;                                    \
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
	ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynq, ETH_ZYNQ_GEM_CAPS, zynq_set_tx_clk_dividers)

#define DT_DRV_COMPAT xlnx_zynq_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQ_GEM_DEVICE)
#undef DT_DRV_COMPAT

/* ZynqMP: the DMA prefetches descriptors */
#define ETH_ZYNQMP_GEM_CAPS                                                                        \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_BD_RD_PREFETCH |                   \
	 CDNS_MACB_CAPS_USRIO_HAS_MII)

#define ETH_ZYNQMP_GEM_DEVICE(n)                                                                   \
	ETH_XLNX_CDNS_MACB_DEVICE(n, eth_zynqmp, ETH_ZYNQMP_GEM_CAPS, zynqmp_set_tx_clk_dividers)

#define DT_DRV_COMPAT xlnx_zynqmp_gem
DT_INST_FOREACH_STATUS_OKAY(ETH_ZYNQMP_GEM_DEVICE)
#undef DT_DRV_COMPAT
