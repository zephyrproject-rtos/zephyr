/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM32H5/H7/H7RS/MP13/N6 specific glue.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_plat, CONFIG_ETHERNET_LOG_LEVEL);

/* be compatible with the HAL-based driver here */
#define DT_DRV_COMPAT st_stm32_ethernet

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>

#include "eth_dwmac_priv.h"
#include "eth_stm32_dwc.h"

#if defined(CONFIG_SOC_SERIES_STM32N6X)
/* The DMA bus master interface is a 64-bit AXI4 interface on this IP */
#define DATA_BUS_WIDTH 64

#define ETH_STM32_DMA_SYSBUS_MODE                                                                  \
	(DMA_SYSBUS_MODE_AAL | DMA_SYSBUS_MODE_FB |                                                \
	 DMA_SYSBUS_MODE_BLEN16 | DMA_SYSBUS_MODE_BLEN8 | DMA_SYSBUS_MODE_BLEN4 |                  \
	 FIELD_PREP(DMA_SYSBUS_MODE_RD_OSR_LMT, 3) | FIELD_PREP(DMA_SYSBUS_MODE_WR_OSR_LMT, 3))
#else
/* The DMA bus master interface is a 32-bit AHB interface on this IP */
#define DATA_BUS_WIDTH 32

#define ETH_STM32_DMA_SYSBUS_MODE (DMA_SYSBUS_MODE_AAL | DMA_SYSBUS_MODE_FB)
#endif

DWMAC_ASSERT_BUFFER_ALIGNMENT(DATA_BUS_WIDTH);

struct eth_stm32_dwc_config {
	/* Has to come first, as the core only knows about this part */
	struct dwmac_config dwmac;
	const struct pinctrl_dev_config *pcfg;
	const struct reset_dt_spec reset;
	const struct stm32_pclken *pclken;
	size_t pclken_cnt;
	/* Selects the PHY interface of this MAC outside of the MAC itself */
	void (*select_phy_interface)(void);
	int (*platform_init)(const struct device *dev);
};

#define ETH_STM32_IS_RMII(n)  DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii)
#define ETH_STM32_IS_RGMII(n) DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii)

#define ETH_STM32_BUILD_ASSERT_PHY_MODE(n)                                                         \
	BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, mii) ||                        \
			     ETH_STM32_IS_RMII(n) ||                                               \
			     ((IS_ENABLED(CONFIG_SOC_SERIES_STM32MP13X) ||                         \
			       IS_ENABLED(CONFIG_SOC_SERIES_STM32N6X)) &&                          \
			      ETH_STM32_IS_RGMII(n)),                                              \
		     "Unsupported PHY connection type")

#if defined(CONFIG_SOC_SERIES_STM32H5X)

#define ETH_STM32_SELECT_PHY_INTERFACE(n)                                                          \
	do {                                                                                       \
		__HAL_RCC_SBS_CLK_ENABLE();                                                        \
		LL_SBS_SetPHYInterface(ETH_STM32_IS_RMII(n) ? LL_SBS_ETH_RMII : LL_SBS_ETH_MII);   \
	} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32H7X)

#define ETH_STM32_SELECT_PHY_INTERFACE(n)                                                          \
	do {                                                                                       \
		__HAL_RCC_SYSCFG_CLK_ENABLE();                                                     \
		LL_SYSCFG_SetPHYInterface(ETH_STM32_IS_RMII(n) ? LL_SYSCFG_ETH_RMII                \
							       : LL_SYSCFG_ETH_MII);               \
	} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32H7RSX)

#define ETH_STM32_SELECT_PHY_INTERFACE(n)                                                          \
	do {                                                                                       \
		__HAL_RCC_SBS_CLK_ENABLE();                                                        \
		LL_SBS_SetEthernetPhy(ETH_STM32_IS_RMII(n) ? LL_SBS_ETH_PHYSEL_RMII                \
							   : LL_SBS_ETH_PHYSEL_GMII_MII);          \
	} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32MP13X)

enum stm32mp1_phy_interface {
	STM32MP1_PHY_MII,
	STM32MP1_PHY_RMII,
	STM32MP1_PHY_RGMII,
};

/* The SYSCFG_PMCSETR bits of one MAC, both MACs have the same layout shifted */
struct stm32mp1_eth_syscfg {
	uint32_t sel_mask;
	uint32_t sel_rmii;
	uint32_t sel_rgmii;
	uint32_t clk_sel;
	uint32_t ref_clk_sel;
};

/*
 * The MP1 series has up to two MACs, ETH1 and ETH2, each with its own set of
 * PHY interface selection bits in SYSCFG_PMCSETR, which is a set-only register
 * with SYSCFG_PMCCLRR as its clear-only counterpart.
 *
 * In RMII mode, the 50 MHz reference clock either comes from the PHY through
 * the ETHx_REF_CLK pad, or the MAC kernel clock (which then has to run at
 * 50 MHz) is driven out to the PHY on the ETHx_CLK pad instead. Likewise in
 * RGMII mode, the 125 MHz clock either comes in on the ETHx_CLK125 pad, or the
 * MAC kernel clock (at 125 MHz) is driven out on the ETHx_CLK pad. This is what
 * the st,ext-phyclk devicetree property selects.
 */
static void stm32mp1_select_phy_interface(const struct stm32mp1_eth_syscfg *syscfg,
					  enum stm32mp1_phy_interface phy_if, bool ext_phyclk)
{
	uint32_t val = 0U;

	switch (phy_if) {
	case STM32MP1_PHY_RMII:
		val = syscfg->sel_rmii | (ext_phyclk ? syscfg->ref_clk_sel : 0U);
		break;
	case STM32MP1_PHY_RGMII:
		val = syscfg->sel_rgmii | (ext_phyclk ? syscfg->clk_sel : 0U);
		break;
	case STM32MP1_PHY_MII:
	default:
		break;
	}

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/*
	 * The PMCCLRR bits read back as the current configuration and clear on
	 * writing 1 (rc_w1), so the read-modify-write the LL_SYSCFG helpers do
	 * on it would clear every other configured bit, including the ones of
	 * the other MAC. Only ever write the bits to clear, or to set for the
	 * PMCSETR counterpart.
	 */
	sys_write32(syscfg->sel_mask | syscfg->clk_sel | syscfg->ref_clk_sel,
		    (mem_addr_t)&SYSCFG->PMCCLRR);
	if (val != 0U) {
		sys_write32(val, (mem_addr_t)&SYSCFG->PMCSETR);
	}
}

static void stm32mp1_eth1_select_phy_interface(enum stm32mp1_phy_interface phy_if, bool ext_phyclk)
{
	static const struct stm32mp1_eth_syscfg eth1_syscfg = {
		.sel_mask = SYSCFG_PMCSETR_ETH1_SEL,
		.sel_rmii = SYSCFG_PMCSETR_ETH1_SEL_2,
		.sel_rgmii = SYSCFG_PMCSETR_ETH1_SEL_0,
		.clk_sel = SYSCFG_PMCSETR_ETH1_CLK_SEL,
		.ref_clk_sel = SYSCFG_PMCSETR_ETH1_REF_CLK_SEL,
	};

	stm32mp1_select_phy_interface(&eth1_syscfg, phy_if, ext_phyclk);
}

#if defined(ETH2_BASE)
static void stm32mp1_eth2_select_phy_interface(enum stm32mp1_phy_interface phy_if, bool ext_phyclk)
{
	static const struct stm32mp1_eth_syscfg eth2_syscfg = {
		.sel_mask = SYSCFG_PMCSETR_ETH2_SEL,
		.sel_rmii = SYSCFG_PMCSETR_ETH2_SEL_2,
		.sel_rgmii = SYSCFG_PMCSETR_ETH2_SEL_0,
		.clk_sel = SYSCFG_PMCSETR_ETH2_CLK_SEL,
		.ref_clk_sel = SYSCFG_PMCSETR_ETH2_REF_CLK_SEL,
	};

	stm32mp1_select_phy_interface(&eth2_syscfg, phy_if, ext_phyclk);
}

#define STM32MP1_IS_ETH2(n) (DT_INST_REG_ADDR(n) == ETH2_BASE)
#else
#define stm32mp1_eth2_select_phy_interface stm32mp1_eth1_select_phy_interface
#define STM32MP1_IS_ETH2(n)                0
#endif

#define STM32MP1_PHY_INTERFACE(n)                                                                  \
	(ETH_STM32_IS_RGMII(n) ? STM32MP1_PHY_RGMII                                                \
			       : (ETH_STM32_IS_RMII(n) ? STM32MP1_PHY_RMII : STM32MP1_PHY_MII))

#define ETH_STM32_SELECT_PHY_INTERFACE(n)                                                          \
	do {                                                                                       \
		if (STM32MP1_IS_ETH2(n)) {                                                         \
			stm32mp1_eth2_select_phy_interface(STM32MP1_PHY_INTERFACE(n),              \
							   DT_INST_PROP(n, st_ext_phyclk));        \
		} else {                                                                           \
			stm32mp1_eth1_select_phy_interface(STM32MP1_PHY_INTERFACE(n),              \
							   DT_INST_PROP(n, st_ext_phyclk));        \
		}                                                                                  \
	} while (0)

#elif defined(CONFIG_SOC_SERIES_STM32N6X)

#define ETH_STM32_SELECT_PHY_INTERFACE(n)                                                          \
	LL_RCC_SetETHPHYInterface(                                                                 \
		ETH_STM32_IS_RGMII(n)                                                              \
			? LL_RCC_ETH1PHY_IF_RGMII                                                  \
			: (ETH_STM32_IS_RMII(n) ? LL_RCC_ETH1PHY_IF_RMII : LL_RCC_ETH1PHY_IF_MII))

#endif

int dwmac_bus_init(const struct device *dev)
{
	const struct eth_stm32_dwc_config *cfg = dev->config;
	int ret;

	/*
	 * Hold the MAC in reset while the pins, the PHY interface mode and the
	 * clocks are configured. The PHY interface mode must be selected while
	 * the MAC is under reset.
	 */
	ret = reset_line_assert_dt(&cfg->reset);
	if (ret != 0) {
		LOG_ERR("Could not assert ethernet reset");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	cfg->select_phy_interface();

	for (size_t n = 0; n < cfg->pclken_cnt; n++) {
		if (IN_RANGE(cfg->pclken[n].bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
			ret = clock_control_on(cfg->dwmac.clock,
					       (clock_control_subsys_t)&cfg->pclken[n]);
		} else {
			ret = clock_control_configure(
				cfg->dwmac.clock, (clock_control_subsys_t)&cfg->pclken[n], NULL);
		}

		if (ret != 0) {
			LOG_ERR("Failed to setup ethernet clock #%zu", n);
			return -EIO;
		}
	}

	ret = reset_line_deassert_dt(&cfg->reset);
	if (ret != 0) {
		LOG_ERR("Could not deassert ethernet reset");
		return ret;
	}

	return 0;
}

#define DESCRIPTOR_ALIGNMENT ((DATA_BUS_WIDTH) / (BITS_PER_BYTE))
#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache_noinit __aligned(DESCRIPTOR_ALIGNMENT)
#else
#define __desc_mem __noinit __aligned(DESCRIPTOR_ALIGNMENT)
#endif

#ifdef CONFIG_MMU
/* The descriptor rings live in the nocache section, which the MMU maps one to one */
#define ETH_STM32_SET_DESCS_PHYS(n, p)                                                             \
	do {                                                                                       \
		(p)->tx_descs_phys = UINT_TO_POINTER(k_mem_phys_addr(eth##n##_tx_descs));          \
		(p)->rx_descs_phys = UINT_TO_POINTER(k_mem_phys_addr(eth##n##_rx_descs));          \
	} while (0)
#else
#define ETH_STM32_SET_DESCS_PHYS(n, p)                                                             \
	do {                                                                                       \
	} while (0)
#endif

int dwmac_platform_init(const struct device *dev)
{
	const struct eth_stm32_dwc_config *cfg = dev->config;

	/* basic configuration for this platform */
	DWMAC_REG_WRITE(MAC_CONF, MAC_CONF_PS | MAC_CONF_FES | MAC_CONF_DM);
	DWMAC_REG_WRITE(DMA_SYSBUS_MODE, ETH_STM32_DMA_SYSBUS_MODE);

	return cfg->platform_init(dev);
}

#define ETH_STM32_PCLKEN_SUBSYS(n, idx) ((clock_control_subsys_t)(eth##n##_pclken + (idx)))

#define ETH_STM32_DWMAC_PTP_CONFIG(n)                                                              \
	.ptp_clock = DEVICE_DT_GET_OR_NULL(DT_INST_CHILD(n, ptp_clock)),                           \
	.ptp_clk = ETH_STM32_PCLKEN_SUBSYS(n, ETH_STM32_PTP_CLK_IDX(n)),

#define ETH_STM32_DWMAC_CONFIG(n)                                                                  \
	{                                                                                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                  \
			.clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),                          \
			.mac_clk = ETH_STM32_PCLKEN_SUBSYS(n, 0),                                  \
			IF_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC, (ETH_STM32_DWMAC_PTP_CONFIG(n)))      \
	}

#define ETH_STM32_DWC_DEVICE(n)                                                                    \
	ETH_STM32_BUILD_ASSERT_PHY_MODE(n);                                                        \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct stm32_pclken eth##n##_pclken[] = STM32_DT_INST_CLOCKS(n);              \
                                                                                                   \
	/* Descriptor rings in uncached memory */                                                  \
	static struct dwmac_dma_desc eth##n##_tx_descs[NB_TX_DESCS] __desc_mem;                    \
	static struct dwmac_dma_desc eth##n##_rx_descs[NB_RX_DESCS] __desc_mem;                    \
                                                                                                   \
	static void eth##n##_select_phy_interface(void)                                            \
	{                                                                                          \
		ETH_STM32_SELECT_PHY_INTERFACE(n);                                                 \
	}                                                                                          \
                                                                                                   \
	static int eth##n##_platform_init(const struct device *dev)                                \
	{                                                                                          \
		const struct net_eth_mac_config mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n);      \
		struct dwmac_priv *p = dev->data;                                                  \
                                                                                                   \
		p->tx_descs = eth##n##_tx_descs;                                                   \
		p->rx_descs = eth##n##_rx_descs;                                                   \
		ETH_STM32_SET_DESCS_PHYS(n, p);                                                    \
                                                                                                   \
		/* set up IRQs (still masked for now) */                                           \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), dwmac_isr,                  \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return eth_stm32_net_eth_mac_load(&mac_cfg, p->mac_addr);                          \
	}                                                                                          \
                                                                                                   \
	static const struct eth_stm32_dwc_config eth##n##_config = {                               \
		.dwmac = ETH_STM32_DWMAC_CONFIG(n),                                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.reset = RESET_DT_SPEC_INST_GET(n),                                                \
		.pclken = eth##n##_pclken,                                                         \
		.pclken_cnt = ARRAY_SIZE(eth##n##_pclken),                                         \
		.select_phy_interface = eth##n##_select_phy_interface,                             \
		.platform_init = eth##n##_platform_init,                                           \
	};                                                                                         \
                                                                                                   \
	static struct dwmac_priv eth##n##_data;                                                    \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, dwmac_probe, NULL, &eth##n##_data, &eth##n##_config,      \
				      CONFIG_ETH_INIT_PRIORITY, &dwmac_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_STM32_DWC_DEVICE)
