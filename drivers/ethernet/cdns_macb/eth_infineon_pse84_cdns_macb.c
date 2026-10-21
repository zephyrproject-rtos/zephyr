/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the Infineon PSOC Edge E84 (PSE84) MCUs.
 */

#define DT_DRV_COMPAT infineon_pse84_gem

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/irq.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <cy_sysclk.h>

#include "eth_cdns_macb_priv.h"

/* The descriptor rings are not cache maintained */
BUILD_ASSERT(!IS_ENABLED(CONFIG_DCACHE) || IS_ENABLED(CONFIG_NOCACHE_MEMORY),
	     "CONFIG_NOCACHE_MEMORY is required with the data cache enabled");

/* The DMA bus master interface of the GEM is a 32-bit AHB interface */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(32);

/* The bus slave, reset and clock of ETH0 are only described for a single instance */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "only one instance is supported");

/* Required by the named DEVICE_MMIO macros, the clock device owns the wrapper register */
#define DEV_CFG(dev)  ((const struct eth_ifx_clk_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_ifx_clk_data *)(dev)->data)

/* Control register of the wrapper (ETH CTL) */
#define IFX_ETH_CTL                0x0
#define IFX_ETH_CTL_MODE           GENMASK(1, 0)
#define IFX_ETH_CTL_MODE_MII       0U
#define IFX_ETH_CTL_MODE_RMII      3U
#define IFX_ETH_CTL_REFCLK_SRC_INT BIT(2)
#define IFX_ETH_CTL_REFCLK_DIV     GENMASK(15, 8)
#define IFX_ETH_CTL_ENABLED        BIT(31)

#define IFX_ETH_REFCLK_DIV_MAX 256U

/* Reference clock of the RMII interface */
#define IFX_ETH_RMII_REF_CLK_RATE MHZ(50)

/* The PERI group key of the PDL, the PERI instance above the group number */
#define IFX_ETH_PERI_GROUP ((CY_MMIO_ETH0_PERI_NR << 8) | CY_MMIO_ETH0_GROUP_NR)

/* The clocks of the MAC provided by the clock device, its subsystems */
enum eth_ifx_clk_id {
	ETH_IFX_CLK_PCLK,
	ETH_IFX_CLK_TX,
	ETH_IFX_CLK_TSU,
};

#define ETH_IFX_CLK_SUBSYS(id) ((clock_control_subsys_t)(uintptr_t)(id))

struct eth_ifx_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

/*
 * Clocks
 *
 * The PSE84 clock dividers have no clock control API, so the clock device
 * of the MAC provides them: the peripheral clock is the bus clock of the
 * PERI group of ETH0, enabling it releases the MAC from reset. The transmit
 * clock is the wrapper: enabling it selects the PHY interface and enables
 * the wrapper, which the GEM registers are only accessible with. The core
 * enables the peripheral clock first and the transmit clock before it
 * touches the GEM registers.
 */

struct eth_ifx_clk_config {
	DEVICE_MMIO_NAMED_ROM(ctl);
	/* Internal source of the RMII reference clock (REF_CLK_INT) */
	struct ifx_cat1_clock ref;
	/* Clock of the time stamp unit (CLK_TSU) */
	struct ifx_cat1_clock tsu;
	bool ref_internal;
	enum cdns_macb_phy_iface phy_iface;
};

struct eth_ifx_clk_data {
	DEVICE_MMIO_NAMED_RAM(ctl);
};

static int eth_ifx_tx_clk_on(const struct device *dev)
{
	const struct eth_ifx_clk_config *cfg = dev->config;
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, ctl) + IFX_ETH_CTL;
	uint32_t mode, divider = 1U, val;

	switch (cfg->phy_iface) {
	case CDNS_MACB_PHY_IFACE_MII:
		/* The PHY is the clock master */
		mode = IFX_ETH_CTL_MODE_MII;
		break;
	case CDNS_MACB_PHY_IFACE_RMII:
		mode = IFX_ETH_CTL_MODE_RMII;
		break;
	default:
		LOG_ERR("unsupported PHY interface %d", cfg->phy_iface);
		return -ENOTSUP;
	}

	if ((mode == IFX_ETH_CTL_MODE_RMII) && cfg->ref_internal) {
		uint32_t ref_rate;

		if (ifx_cat1_utils_peri_pclk_assign_divider(PCLK_ETH0_REF_CLK_INT, &cfg->ref) !=
		    CY_RSLT_SUCCESS) {
			LOG_ERR("unable to connect the reference clock");
			return -EIO;
		}

		ref_rate = ifx_cat1_utils_peri_pclk_get_frequency(PCLK_ETH0_REF_CLK_INT, &cfg->ref);
		if ((ref_rate == 0U) || ((ref_rate % IFX_ETH_RMII_REF_CLK_RATE) != 0U) ||
		    ((ref_rate / IFX_ETH_RMII_REF_CLK_RATE) > IFX_ETH_REFCLK_DIV_MAX)) {
			LOG_ERR("%u Hz is not derivable from the %u Hz reference clock",
				IFX_ETH_RMII_REF_CLK_RATE, ref_rate);
			return -EINVAL;
		}

		divider = ref_rate / IFX_ETH_RMII_REF_CLK_RATE;
	}

	/* Keep the wrapper enabled once it is, the GEM becomes inaccessible otherwise */
	val = sys_read32(reg) & IFX_ETH_CTL_ENABLED;
	val |= FIELD_PREP(IFX_ETH_CTL_MODE, mode) |
	       FIELD_PREP(IFX_ETH_CTL_REFCLK_DIV, divider - 1U);
	if ((mode == IFX_ETH_CTL_MODE_RMII) && cfg->ref_internal) {
		val |= IFX_ETH_CTL_REFCLK_SRC_INT;
	}
	sys_write32(val, reg);
	sys_write32(val | IFX_ETH_CTL_ENABLED, reg);

	LOG_DBG("wrapper mode %u, reference clock divided by %u", mode, divider);

	return 0;
}

static int eth_ifx_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct eth_ifx_clk_config *cfg = dev->config;

	switch ((uintptr_t)sys) {
	case ETH_IFX_CLK_PCLK:
		Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_ETH0_PERI_NR, CY_MMIO_ETH0_GROUP_NR,
					     CY_MMIO_ETH0_SLAVE_NR, CY_MMIO_ETH0_CLK_HF_NR);
		return 0;
	case ETH_IFX_CLK_TX:
		return eth_ifx_tx_clk_on(dev);
	case ETH_IFX_CLK_TSU:
		if (ifx_cat1_utils_peri_pclk_assign_divider(PCLK_ETH0_CLK_TSU, &cfg->tsu) !=
		    CY_RSLT_SUCCESS) {
			LOG_ERR("unable to connect the time stamp unit clock");
			return -EIO;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static int eth_ifx_clk_get_rate(const struct device *dev, clock_control_subsys_t sys,
				uint32_t *rate)
{
	const struct eth_ifx_clk_config *cfg = dev->config;

	switch ((uintptr_t)sys) {
	case ETH_IFX_CLK_PCLK:
		*rate = Cy_SysClk_ClkHfGetFrequency(CY_MMIO_ETH0_CLK_HF_NR) /
			(Cy_SysClk_PeriGroupGetDivider(IFX_ETH_PERI_GROUP) + 1U);
		break;
	case ETH_IFX_CLK_TSU:
		*rate = ifx_cat1_utils_peri_pclk_get_frequency(PCLK_ETH0_CLK_TSU, &cfg->tsu);
		break;
	default:
		return -ENOTSUP;
	}

	return (*rate != 0U) ? 0 : -EIO;
}

static int eth_ifx_clk_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, ctl, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, eth_ifx_clk_api) = {
	.on = eth_ifx_clk_on,
	.get_rate = eth_ifx_clk_get_rate,
};

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_ifx_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	/* The SoC has no MMU, the DMA sees the rings at their CPU address */
	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

#define ETH_IFX_CLOCK_NODE(n, name) DT_INST_CLOCKS_CTLR_BY_NAME(n, name)

#define ETH_IFX_CLOCK_IS_OKAY(n, name)                                                             \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, name),                                              \
		    (DT_NODE_HAS_STATUS_OKAY(ETH_IFX_CLOCK_NODE(n, name))), (0))

/* A peripheral clock divider of the clocks property, selected by its name */
#define ETH_IFX_PERI_DIV(n, name)                                                                  \
	COND_CODE_1(ETH_IFX_CLOCK_IS_OKAY(n, name),                                                \
		    ({                                                                             \
			    .block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                             \
				    DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 0),    \
				    DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 1),    \
				    DT_PROP(ETH_IFX_CLOCK_NODE(n, name), div_type)),               \
			    .channel = DT_PROP(ETH_IFX_CLOCK_NODE(n, name), channel),              \
			    .instance = DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 0),\
			    .group = DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 1),   \
		    }),                                                                            \
		    ({0}))

#define ETH_IFX_CLK(n, id) {.dev = DEVICE_GET(eth##n##_clk), .subsys = ETH_IFX_CLK_SUBSYS(id)}

/* Without its divider, the time stamp unit is taken to run from the peripheral clock */
#define ETH_IFX_TSU_CLK(n)                                                                         \
	COND_CODE_1(ETH_IFX_CLOCK_IS_OKAY(n, tsu_clk), (ETH_IFX_CLK(n, ETH_IFX_CLK_TSU)), ({0}))

/* The DMA prefetches descriptors */
#define ETH_IFX_CDNS_MACB_CAPS CDNS_MACB_CAPS_BD_RD_PREFETCH

#define ETH_IFX_PERI_INST(n, name)                                                             \
	COND_CODE_1(ETH_IFX_CLOCK_IS_OKAY(n, name),                                             \
		    (DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 0)), (0))

#define ETH_IFX_PERI_GRP(n, name)                                                              \
	COND_CODE_1(ETH_IFX_CLOCK_IS_OKAY(n, name),                                             \
		    (DT_PROP_BY_IDX(ETH_IFX_CLOCK_NODE(n, name), peri_group, 1)), (0))

#define ETH_IFX_CDNS_MACB_DEVICE(n)                                                                \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
	BUILD_ASSERT(DT_PROP(ETH_IFX_CLOCK_NODE(n, pclk), instance) == CY_MMIO_ETH0_CLK_HF_NR,     \
		     "pclk must be the clk_hf root clock of the PERI group of ETH0");              \
	BUILD_ASSERT(!ETH_IFX_CLOCK_IS_OKAY(n, ref_clk) ||                                      \
		     ((ETH_IFX_PERI_INST(n, ref_clk) == 0U) &&                                  \
		      (ETH_IFX_PERI_GRP(n, ref_clk) == 3U)),                                    \
		     "ref_clk must be a PERI0 group 3 clock divider");                          \
	BUILD_ASSERT(!ETH_IFX_CLOCK_IS_OKAY(n, tsu_clk) ||                                      \
		     ((ETH_IFX_PERI_INST(n, tsu_clk) == 0U) &&                                  \
		      (ETH_IFX_PERI_GRP(n, tsu_clk) == 4U)),                                    \
		     "tsu_clk must be a PERI0 group 4 clock divider");                          \
	BUILD_ASSERT(!IS_ENABLED(CONFIG_PTP_CLOCK_CDNS_MACB) ||                                 \
		     !DT_NODE_HAS_STATUS_OKAY(DT_INST_CHILD(n, ptp_clock)) ||                   \
		     ETH_IFX_CLOCK_IS_OKAY(n, tsu_clk),                                         \
		     "an enabled tsu_clk (PERI0 group 4) is required when PTP is enabled");     \
	BUILD_ASSERT((CDNS_MACB_DT_INST_PHY_IFACE(n) == CDNS_MACB_PHY_IFACE_MII) ||                \
			     (CDNS_MACB_DT_INST_PHY_IFACE(n) == CDNS_MACB_PHY_IFACE_RMII),         \
		     "only the MII and RMII interfaces are supported");                           \
                                                                                                   \
	static const struct eth_ifx_clk_config eth##n##_clk_config = {                             \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ctl, DT_DRV_INST(n)),                           \
		.ref = ETH_IFX_PERI_DIV(n, ref_clk),                                               \
		.tsu = ETH_IFX_PERI_DIV(n, tsu_clk),                                               \
		.ref_internal = ETH_IFX_CLOCK_IS_OKAY(n, ref_clk),                                 \
		.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                                       \
	};                                                                                         \
                                                                                                   \
	static struct eth_ifx_clk_data eth##n##_clk_data;                                          \
                                                                                                   \
	DEVICE_DEFINE(eth##n##_clk, DEVICE_DT_NAME(DT_DRV_INST(n)) "_clk", eth_ifx_clk_init, NULL, \
		      &eth##n##_clk_data, &eth##n##_clk_config, PRE_KERNEL_1,                      \
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &eth_ifx_clk_api);                       \
                                                                                                   \
	CDNS_MACB_DT_INST_PINCTRL_DEFINE(n)                                                        \
                                                                                                   \
	static struct cdns_macb_rings eth##n##_rings __nocache_noinit                              \
		__aligned(CDNS_MACB_RINGS_ALIGN);                                                  \
                                                                                                   \
	static int eth##n##_platform_init(const struct device *dev)                                \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		/* set up the IRQ of queue 0 (still masked for now), the other queues are idle */  \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_macb_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_ifx_cdns_macb_config eth##n##_config = {                           \
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
				[CDNS_MACB_CLK_PCLK] = ETH_IFX_CLK(n, ETH_IFX_CLK_PCLK),           \
				[CDNS_MACB_CLK_TX] = ETH_IFX_CLK(n, ETH_IFX_CLK_TX),               \
				[CDNS_MACB_CLK_TSU] = ETH_IFX_TSU_CLK(n),                          \
			},                                                                         \
			.caps = ETH_IFX_CDNS_MACB_CAPS,                                            \
			.dma_burst_length = 4,                                                     \
			.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                               \
		},                                                                                 \
		.platform_init = eth##n##_platform_init,                                           \
	};                                                                                         \
                                                                                                   \
	static struct cdns_macb_priv eth##n##_data;                                                \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, cdns_macb_probe, NULL, &eth##n##_data, &eth##n##_config,  \
				      CONFIG_ETH_INIT_PRIORITY, &cdns_macb_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_IFX_CDNS_MACB_DEVICE)
