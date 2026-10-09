/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cadence GEM glue for the Infineon CAT1C (TRAVEO T2G) MCUs.
 */

#define DT_DRV_COMPAT infineon_cat1c_gem

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_plat, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "eth_cdns_macb_priv.h"

/* The descriptor rings are not cache maintained, they need an uncached region */
BUILD_ASSERT(IS_ENABLED(CONFIG_NOCACHE_MEMORY), "CONFIG_NOCACHE_MEMORY is required");

/* The DMA bus master interface of the GEM is a 64-bit AXI interface */
CDNS_MACB_ASSERT_BUFFER_ALIGNMENT(64);

/* Required by the named DEVICE_MMIO macros, the TX clock device owns the register */
#define DEV_CFG(dev)  ((const struct eth_ifx_tx_clk_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_ifx_tx_clk_data *)(dev)->data)

/*
 * Control register of the wrapper (MXETH CTL)
 *
 * ETH_MODE selects the PHY interface and with it the source of the transmit
 * and receive clock domains of the GEM. For RGMII, GMII and RMII the
 * transmit clock is the reference clock divided by REFCLK_DIV + 1; the
 * reference clock is the REF_CLK input pin or the internal clock.
 */
#define IFX_ETH_CTL                0x0
#define IFX_ETH_CTL_MODE           GENMASK(1, 0)
#define IFX_ETH_CTL_MODE_MII       0U
#define IFX_ETH_CTL_MODE_GMII      1U
#define IFX_ETH_CTL_MODE_RGMII     2U
#define IFX_ETH_CTL_MODE_RMII      3U
#define IFX_ETH_CTL_REFCLK_SRC_INT BIT(2)
#define IFX_ETH_CTL_REFCLK_DIV     GENMASK(15, 8)
#define IFX_ETH_CTL_ENABLED        BIT(31)

#define IFX_ETH_REFCLK_DIV_MAX 256U

/* Transmit clock of the RMII interface */
#define IFX_ETH_RMII_TX_CLK_RATE MHZ(50)

struct eth_ifx_cdns_macb_config {
	/* Has to come first, as the core only knows about this part */
	struct cdns_macb_config macb;
	int (*platform_init)(const struct device *dev);
};

/*
 * TX clock
 *
 * The control register of the wrapper is a clock of its own, the transmit
 * clock of the MAC. It is also where the wrapper is set up and enabled: the
 * core enables its clocks first thing, before it touches the GEM registers,
 * which are only accessible with the wrapper enabled.
 */

struct eth_ifx_tx_clk_config {
	DEVICE_MMIO_NAMED_ROM(ctl);
	/* The source of the reference clock divider */
	struct cdns_macb_clock ref;
	/* The source is the internal clock, not the REF_CLK pin */
	bool ref_internal;
	enum cdns_macb_phy_iface phy_iface;
};

struct eth_ifx_tx_clk_data {
	DEVICE_MMIO_NAMED_RAM(ctl);
};

/* The divider of the reference clock giving a transmit clock rate */
static int eth_ifx_tx_clk_div(const struct device *dev, uint32_t rate_hz, uint32_t *divider)
{
	const struct eth_ifx_tx_clk_config *cfg = dev->config;
	uint32_t ref_rate;
	int ret;

	ret = clock_control_get_rate(cfg->ref.dev, cfg->ref.subsys, &ref_rate);
	if (ret < 0) {
		LOG_ERR("unable to get the rate of the reference clock (%d)", ret);
		return ret;
	}

	if ((rate_hz == 0U) || ((ref_rate % rate_hz) != 0U) ||
	    ((ref_rate / rate_hz) > IFX_ETH_REFCLK_DIV_MAX)) {
		LOG_ERR("%u Hz is not derivable from the %u Hz reference clock", rate_hz, ref_rate);
		return -EINVAL;
	}

	*divider = ref_rate / rate_hz;

	return 0;
}

static int eth_ifx_tx_clk_on(const struct device *dev, __unused clock_control_subsys_t sys)
{
	const struct eth_ifx_tx_clk_config *cfg = dev->config;
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, ctl) + IFX_ETH_CTL;
	uint32_t mode, rate_hz, divider = 1U, val;
	int ret;

	switch (cfg->phy_iface) {
	case CDNS_MACB_PHY_IFACE_MII:
		/* The PHY is the clock master */
		mode = IFX_ETH_CTL_MODE_MII;
		rate_hz = 0U;
		break;
	case CDNS_MACB_PHY_IFACE_GMII:
		mode = IFX_ETH_CTL_MODE_GMII;
		rate_hz = MHZ(125);
		break;
	case CDNS_MACB_PHY_IFACE_RGMII:
		mode = IFX_ETH_CTL_MODE_RGMII;
		rate_hz = MHZ(125);
		break;
	case CDNS_MACB_PHY_IFACE_RMII:
		mode = IFX_ETH_CTL_MODE_RMII;
		rate_hz = IFX_ETH_RMII_TX_CLK_RATE;
		break;
	default:
		LOG_ERR("unsupported PHY interface %d", cfg->phy_iface);
		return -ENOTSUP;
	}

	if (rate_hz != 0U) {
		ret = eth_ifx_tx_clk_div(dev, rate_hz, &divider);
		if (ret < 0) {
			return ret;
		}
	}

	/* Keep the wrapper enabled once it is, the GEM becomes inaccessible otherwise */
	val = sys_read32(reg) & IFX_ETH_CTL_ENABLED;
	val |= FIELD_PREP(IFX_ETH_CTL_MODE, mode) |
	       FIELD_PREP(IFX_ETH_CTL_REFCLK_DIV, divider - 1U);
	if (cfg->ref_internal) {
		val |= IFX_ETH_CTL_REFCLK_SRC_INT;
	}
	sys_write32(val, reg);
	sys_write32(val | IFX_ETH_CTL_ENABLED, reg);

	LOG_DBG("wrapper mode %u, reference clock %s divided by %u", mode,
		cfg->ref_internal ? "internal" : "REF_CLK", divider);

	return 0;
}

static int eth_ifx_tx_clk_off(__unused const struct device *dev,
			      __unused clock_control_subsys_t sys)
{
	return 0;
}

static int eth_ifx_tx_clk_get_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				   uint32_t *rate)
{
	const struct eth_ifx_tx_clk_config *cfg = dev->config;
	uint32_t val = sys_read32(DEVICE_MMIO_NAMED_GET(dev, ctl) + IFX_ETH_CTL);
	uint32_t ref_rate;
	int ret;

	ret = clock_control_get_rate(cfg->ref.dev, cfg->ref.subsys, &ref_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = ref_rate / (FIELD_GET(IFX_ETH_CTL_REFCLK_DIV, val) + 1U);

	return 0;
}

static int eth_ifx_tx_clk_set_rate(const struct device *dev, __unused clock_control_subsys_t sys,
				   clock_control_subsys_rate_t rate)
{
	mm_reg_t reg = DEVICE_MMIO_NAMED_GET(dev, ctl) + IFX_ETH_CTL;
	uint32_t rate_hz = (uint32_t)(uintptr_t)rate;
	uint32_t divider, val;
	int ret;

	ret = eth_ifx_tx_clk_div(dev, rate_hz, &divider);
	if (ret < 0) {
		return ret;
	}

	val = sys_read32(reg) & ~IFX_ETH_CTL_REFCLK_DIV;
	val |= FIELD_PREP(IFX_ETH_CTL_REFCLK_DIV, divider - 1U);
	sys_write32(val, reg);

	return 0;
}

static int eth_ifx_tx_clk_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, ctl, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, eth_ifx_tx_clk_api) = {
	.on = eth_ifx_tx_clk_on,
	.off = eth_ifx_tx_clk_off,
	.get_rate = eth_ifx_tx_clk_get_rate,
	.set_rate = eth_ifx_tx_clk_set_rate,
};

int cdns_macb_platform_init(const struct device *dev)
{
	const struct eth_ifx_cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	/* The rings live in the nocache section, the SoC has no MMU */
	p->rings_phys = POINTER_TO_UINT(cfg->macb.rings);

	return cfg->platform_init(dev);
}

/*
 * Clocks
 *
 * The clk_hf root clocks and the clk_peri peripheral clock of the SoC have
 * no device of their own, they are subsystems of the clock controller device,
 * the clocks node. Any other clock, like the fixed-clock of an oscillator, is
 * a device of its own.
 */

#define ETH_IFX_CLOCK_NODE(n, name) DT_INST_CLOCKS_CTLR_BY_NAME(n, name)

#define ETH_IFX_CLOCK_IS_HF(n, name)                                                               \
	DT_NODE_HAS_COMPAT(ETH_IFX_CLOCK_NODE(n, name), infineon_cat1c_clk_hf)

#define ETH_IFX_CLOCK_IS_PERI(n, name)                                                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(clk_peri)),                                        \
		    (DT_SAME_NODE(ETH_IFX_CLOCK_NODE(n, name), DT_NODELABEL(clk_peri))), (0))

#define ETH_IFX_CLOCK_IS_SOC(n, name)                                                              \
	UTIL_OR(ETH_IFX_CLOCK_IS_HF(n, name), ETH_IFX_CLOCK_IS_PERI(n, name))

#define ETH_IFX_SOC_CLOCK_SUBSYS(n, name) eth##n##_##name##_subsys

/* The subsystem of a clk_hf or clk_peri clock of the clocks property, nothing for others */
#define ETH_IFX_SOC_CLOCK_SUBSYS_DEFINE(n, name)                                                   \
	IF_ENABLED(UTIL_AND(DT_INST_CLOCKS_HAS_NAME(n, name), ETH_IFX_CLOCK_IS_SOC(n, name)),      \
		   (static struct ifx_clk ETH_IFX_SOC_CLOCK_SUBSYS(n, name) = {                    \
			    .clk = COND_CODE_1(ETH_IFX_CLOCK_IS_PERI(n, name), (IFX_CLK_PERI),     \
					       (IFX_CLK_HF)),                                      \
			    .clk_id = COND_CODE_1(ETH_IFX_CLOCK_IS_PERI(n, name), (0),             \
						  (DT_REG_ADDR(ETH_IFX_CLOCK_NODE(n, name)))),     \
		    };))

/* A clock of the clocks property of an instance, selected by its name */
#define ETH_IFX_CLOCK(n, name)                                                                     \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, name),                                              \
		    (COND_CODE_1(ETH_IFX_CLOCK_IS_SOC(n, name),                                    \
				 ({                                                                \
					 .dev = DEVICE_DT_GET(DT_NODELABEL(clocks)),               \
					 .subsys = &ETH_IFX_SOC_CLOCK_SUBSYS(n, name),             \
				 }),                                                               \
				 ({                                                                \
					 .dev = DEVICE_DT_GET(ETH_IFX_CLOCK_NODE(n, name)),        \
					 .subsys = NULL,                                           \
				 }))),                                                             \
		    ({0}))

/* The DMA prefetches descriptors */
#define ETH_IFX_CDNS_MACB_CAPS                                                                     \
	(CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE | CDNS_MACB_CAPS_BD_RD_PREFETCH)

#define ETH_IFX_CDNS_MACB_DEVICE(n)                                                                \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pclk), "the pclk clock is required");              \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, ref_clk), "the ref_clk clock is required");        \
	BUILD_ASSERT(!ETH_IFX_CLOCK_IS_HF(n, ref_clk) ||                                           \
			     (!CDNS_MACB_DT_INST_IS_RGMII(n) &&                                    \
			      !DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii)),              \
		     "the internal reference clock is only available for RMII");                  \
                                                                                                   \
	ETH_IFX_SOC_CLOCK_SUBSYS_DEFINE(n, pclk)                                                   \
	ETH_IFX_SOC_CLOCK_SUBSYS_DEFINE(n, ref_clk)                                                \
	ETH_IFX_SOC_CLOCK_SUBSYS_DEFINE(n, tsu_clk)                                                \
                                                                                                   \
	static const struct eth_ifx_tx_clk_config eth##n##_tx_clk_config = {                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ctl, DT_DRV_INST(n)),                           \
		.ref = ETH_IFX_CLOCK(n, ref_clk),                                                  \
		.ref_internal = ETH_IFX_CLOCK_IS_HF(n, ref_clk),                                   \
		.phy_iface = CDNS_MACB_DT_INST_PHY_IFACE(n),                                       \
	};                                                                                         \
                                                                                                   \
	static struct eth_ifx_tx_clk_data eth##n##_tx_clk_data;                                    \
                                                                                                   \
	DEVICE_DEFINE(eth##n##_tx_clk, DEVICE_DT_NAME(DT_DRV_INST(n)) "_tx_clk",                   \
		      eth_ifx_tx_clk_init, NULL, &eth##n##_tx_clk_data, &eth##n##_tx_clk_config,   \
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &eth_ifx_tx_clk_api);      \
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
			.rings = &eth##n##_rings,                                                  \
			.clks = {                                                                  \
				[CDNS_MACB_CLK_PCLK] = ETH_IFX_CLOCK(n, pclk),                     \
				[CDNS_MACB_CLK_TX] = {.dev = DEVICE_GET(eth##n##_tx_clk)},         \
				[CDNS_MACB_CLK_TSU] = ETH_IFX_CLOCK(n, tsu_clk),                   \
			},                                                                         \
			.caps = ETH_IFX_CDNS_MACB_CAPS,                                            \
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

DT_INST_FOREACH_STATUS_OKAY(ETH_IFX_CDNS_MACB_DEVICE)
