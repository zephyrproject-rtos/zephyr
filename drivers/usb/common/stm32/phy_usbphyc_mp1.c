/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* STM32MP1 USBPHYC USB HS PHY controller driver */
#define DT_DRV_COMPAT st_stm32mp1_usbphyc

#include <soc.h>

#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbphyc_stm32mp1, CONFIG_STM32_USBPHYC_MP1_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Only one USBPHYC instance is supported");

/* Register offsets */
#define STM32_USBPHYC_PLL        0x000U
#define STM32_USBPHYC_MISC       0x008U
#define STM32_USBPHYC_TUNE(port) (0x10CU + (port) * 0x100U)

/* STM32_USBPHYC_PLL */
#define STM32_USBPHYC_PLL_PLLNDIV    GENMASK(6, 0)
#define STM32_USBPHYC_PLL_PLLODF     GENMASK(9, 7)
#define STM32_USBPHYC_PLL_PLLFRACIN  GENMASK(25, 10)
#define STM32_USBPHYC_PLL_PLLEN      BIT(26)
#define STM32_USBPHYC_PLL_PLLSTRB    BIT(27)
#define STM32_USBPHYC_PLL_PLLSTRBYP  BIT(28)
#define STM32_USBPHYC_PLL_PLLFRACCTL BIT(29)
#define STM32_USBPHYC_PLL_PLLDITHEN0 BIT(30)
#define STM32_USBPHYC_PLL_PLLDITHEN1 BIT(31)

/* STM32_USBPHYC_MISC */
#define STM32_USBPHYC_MISC_SWITHOST BIT(0)

/* USBPHYC_TUNEx */
#define STM32_USBPHYC_TUNE_INCURREN      BIT(0)
#define STM32_USBPHYC_TUNE_INCURRINT     BIT(1)
#define STM32_USBPHYC_TUNE_LFSCAPEN      BIT(2)
#define STM32_USBPHYC_TUNE_HSDRVSLEW     BIT(3)
#define STM32_USBPHYC_TUNE_HSDRVDCCUR    BIT(4)
#define STM32_USBPHYC_TUNE_HSDRVDCLEV    BIT(5)
#define STM32_USBPHYC_TUNE_HSDRVCURINCR  BIT(6)
#define STM32_USBPHYC_TUNE_FSDRVRFADJ    BIT(7)
#define STM32_USBPHYC_TUNE_HSDRVRFRED    BIT(8)
#define STM32_USBPHYC_TUNE_HSDRVCHKITRM  GENMASK(12, 9)
#define STM32_USBPHYC_TUNE_HSDRVCHKZTRM  GENMASK(14, 13)
#define STM32_USBPHYC_TUNE_OTPCOMP       GENMASK(19, 15)
#define STM32_USBPHYC_TUNE_SQLCHCTL      GENMASK(21, 20)
#define STM32_USBPHYC_TUNE_HDRXGNEQEN    BIT(22)
#define STM32_USBPHYC_TUNE_HSRXOFF       GENMASK(24, 23)
#define STM32_USBPHYC_TUNE_HSFALLPREEM   BIT(25)
#define STM32_USBPHYC_TUNE_SHTCCTCTLPROT BIT(26)
#define STM32_USBPHYC_TUNE_STAGSEL       BIT(27)

/* PLL: FVCO = INFF * 2 * (NDIV + FRACIN / 2^16), FVCO fixed at 2880 MHz */
#define STM32_USBPHYC_PLL_FVCO_HZ          2880000000ULL
#define STM32_USBPHYC_PLL_INFF_MIN_HZ      19200000U
#define STM32_USBPHYC_PLL_INFF_MAX_HZ      38400000U
#define STM32_USBPHYC_PLL_LOCK_TIME_US     300U
#define STM32_USBPHYC_PLL_PWR_DOWN_US      10U
#define STM32_USBPHYC_PWR_READY_TIMEOUT_US 10000U

/* HS driver DC level values of the st,tune-hs-dc-level property */
#define STM32_USBPHYC_DC_PLUS_10_TO_14_MV 2U
#define STM32_USBPHYC_DC_MINUS_5_TO_7_MV  3U

struct usbphyc_port_config {
	uint8_t index;
	uint32_t tune;
};

struct usbphyc_config {
	uintptr_t base;
	const struct stm32_pclken *pclken;
	size_t pclken_len;
	struct reset_dt_spec reset;
	const struct usbphyc_port_config *ports;
	size_t num_ports;
	bool port1_host;
};

static int usbphyc_wait_flag(uintptr_t reg, uint32_t mask, bool set)
{
	for (uint32_t i = 0U; i < STM32_USBPHYC_PWR_READY_TIMEOUT_US; i += 10U) {
		if (((sys_read32(reg) & mask) != 0U) == set) {
			return 0;
		}

		k_busy_wait(10U);
	}

	return -ETIMEDOUT;
}

static int usbphyc_pwr_enable(void)
{
	uintptr_t cr3 = (uintptr_t)&PWR->CR3;
	int ret;

	/* VDD3V3_USBHS is supplied externally, check that it is present */
	LL_PWR_EnableUSBVoltageDetector();
	ret = usbphyc_wait_flag(cr3, PWR_CR3_USB33RDY, true);
	if (ret < 0) {
		LOG_ERR("VDD3V3_USBHS not ready, check the external USB supply");
		return ret;
	}

	/* Internal regulators supplying the PHY analog domain */
	LL_PWR_Enable1V1Regulator();
	ret = usbphyc_wait_flag(cr3, PWR_CR3_REG11RDY, true);
	if (ret < 0) {
		LOG_ERR("1.1 V USB regulator not ready");
		return ret;
	}

	LL_PWR_Enable1V8Regulator();
	ret = usbphyc_wait_flag(cr3, PWR_CR3_REG18RDY, true);
	if (ret < 0) {
		LOG_ERR("1.8 V USB regulator not ready");
		return ret;
	}

	return 0;
}

static int usbphyc_pll_enable(const struct usbphyc_config *cfg)
{
	uintptr_t pll_reg = cfg->base + STM32_USBPHYC_PLL;
	uint32_t clk_rate = LL_RCC_GetUSBPHYClockFreq(LL_RCC_USBPHY_CLKSOURCE);
	uint64_t ndiv, frac;
	uint32_t pll;
	int ret;

	if (clk_rate < STM32_USBPHYC_PLL_INFF_MIN_HZ || clk_rate > STM32_USBPHYC_PLL_INFF_MAX_HZ) {
		LOG_ERR("PLL reference clock %u Hz out of range", clk_rate);
		return -EINVAL;
	}

	ndiv = STM32_USBPHYC_PLL_FVCO_HZ / (2ULL * clk_rate);
	frac = (STM32_USBPHYC_PLL_FVCO_HZ << 16) / (2ULL * clk_rate) - (ndiv << 16);

	pll = STM32_USBPHYC_PLL_PLLDITHEN1 | STM32_USBPHYC_PLL_PLLDITHEN0 |
	      STM32_USBPHYC_PLL_PLLSTRBYP | FIELD_PREP(STM32_USBPHYC_PLL_PLLNDIV, (uint32_t)ndiv);
	if (frac != 0U) {
		pll |= STM32_USBPHYC_PLL_PLLFRACCTL |
		       FIELD_PREP(STM32_USBPHYC_PLL_PLLFRACIN, (uint32_t)frac);
	}

	/* The PLL must be disabled while it is reconfigured */
	if ((sys_read32(pll_reg) & STM32_USBPHYC_PLL_PLLEN) != 0U) {
		sys_clear_bits(pll_reg, STM32_USBPHYC_PLL_PLLEN);
		ret = usbphyc_wait_flag(pll_reg, STM32_USBPHYC_PLL_PLLEN, false);
		if (ret < 0) {
			LOG_ERR("PLL not disabled");
			return ret;
		}

		k_busy_wait(STM32_USBPHYC_PLL_PWR_DOWN_US);
	}

	sys_write32(pll, pll_reg);
	sys_set_bits(pll_reg, STM32_USBPHYC_PLL_PLLEN);

	/* Wait for the maximum lock time, there is no lock status flag */
	k_busy_wait(STM32_USBPHYC_PLL_LOCK_TIME_US);

	LOG_DBG("PLL reference %u Hz, ndiv %u, frac %u", clk_rate, (uint32_t)ndiv, (uint32_t)frac);

	return 0;
}

static void usbphyc_port_tune(const struct usbphyc_config *cfg,
			      const struct usbphyc_port_config *port)
{
	uintptr_t tune_reg = cfg->base + STM32_USBPHYC_TUNE(port->index);
	uint32_t tune;

	/* Keep the OTP compensation code programmed by hardware */
	tune = sys_read32(tune_reg) & STM32_USBPHYC_TUNE_OTPCOMP;
	sys_write32(tune | port->tune, tune_reg);

	LOG_DBG("Port %u tune 0x%08x", port->index, sys_read32(tune_reg));
}

static int usbphyc_init(const struct device *dev)
{
	const struct usbphyc_config *cfg = dev->config;
	const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	if (!device_is_ready(rcc)) {
		LOG_ERR("Clock controller not ready");
		return -ENODEV;
	}

	ret = clock_control_on(rcc, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed to enable USBPHYC clock %d", ret);
		return ret;
	}

	if (cfg->pclken_len > 1U) {
		ret = clock_control_configure(rcc, (clock_control_subsys_t)&cfg->pclken[1], NULL);
		if (ret < 0) {
			LOG_ERR("Failed to configure USBPHYC reference clock %d", ret);
			return ret;
		}
	}

	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0) {
		LOG_ERR("Failed to reset USBPHYC %d", ret);
		return ret;
	}

	ret = usbphyc_pwr_enable();
	if (ret < 0) {
		return ret;
	}

	ret = usbphyc_pll_enable(cfg);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0U; i < cfg->num_ports; i++) {
		usbphyc_port_tune(cfg, &cfg->ports[i]);
	}

	/* Route PHY port 1 to either the USB host block or the OTG controller */
	if (cfg->port1_host) {
		sys_set_bits(cfg->base + STM32_USBPHYC_MISC, STM32_USBPHYC_MISC_SWITHOST);
	} else {
		sys_clear_bits(cfg->base + STM32_USBPHYC_MISC, STM32_USBPHYC_MISC_SWITHOST);
	}

	return 0;
}

/* HS driver DC level, see the st,tune-hs-dc-level property */
#define STM32_USBPHYC_TUNE_DC_LEVEL(val)                                                           \
	((val) == STM32_USBPHYC_DC_MINUS_5_TO_7_MV ? STM32_USBPHYC_TUNE_HSDRVDCCUR                 \
	 : (val) != 0U                                                                             \
		 ? (STM32_USBPHYC_TUNE_HSDRVCURINCR |                                              \
		    ((val) == STM32_USBPHYC_DC_PLUS_10_TO_14_MV ? STM32_USBPHYC_TUNE_HSDRVDCLEV    \
								: 0U))                             \
		 : 0U)

#define STM32_USBPHYC_TUNE_BOOST(val)                                                              \
	((val) != 0U ? (STM32_USBPHYC_TUNE_INCURREN |                                              \
			((val) == 2000U ? STM32_USBPHYC_TUNE_INCURRINT : 0U))                      \
		     : 0U)

#define USBPHYC_PORT_TUNE(node)                                                                    \
	(STM32_USBPHYC_TUNE_BOOST(DT_PROP_OR(node, st_current_boost_microamp, 0)) |                \
	 (DT_PROP(node, st_no_lsfs_fb_cap) ? 0U : STM32_USBPHYC_TUNE_LFSCAPEN) |                   \
	 (DT_PROP(node, st_decrease_hs_slew_rate) ? STM32_USBPHYC_TUNE_HSDRVSLEW : 0U) |           \
	 STM32_USBPHYC_TUNE_DC_LEVEL(DT_PROP_OR(node, st_tune_hs_dc_level, 0)) |                   \
	 (DT_PROP(node, st_enable_fs_rftime_tuning) ? STM32_USBPHYC_TUNE_FSDRVRFADJ : 0U) |        \
	 (DT_PROP(node, st_enable_hs_rftime_reduction) ? STM32_USBPHYC_TUNE_HSDRVRFRED : 0U) |     \
	 FIELD_PREP(STM32_USBPHYC_TUNE_HSDRVCHKITRM, DT_PROP_OR(node, st_trim_hs_current, 0)) |    \
	 FIELD_PREP(STM32_USBPHYC_TUNE_HSDRVCHKZTRM, DT_PROP_OR(node, st_trim_hs_impedance, 0)) |  \
	 FIELD_PREP(STM32_USBPHYC_TUNE_SQLCHCTL, DT_PROP_OR(node, st_tune_squelch_level, 0)) |     \
	 (DT_PROP(node, st_enable_hs_rx_gain_eq) ? STM32_USBPHYC_TUNE_HDRXGNEQEN : 0U) |           \
	 FIELD_PREP(STM32_USBPHYC_TUNE_HSRXOFF, DT_PROP_OR(node, st_tune_hs_rx_offset, 0)) |       \
	 (DT_PROP(node, st_no_hs_ftime_ctrl) ? STM32_USBPHYC_TUNE_HSFALLPREEM : 0U) |              \
	 (DT_PROP(node, st_no_lsfs_sc) ? 0U : STM32_USBPHYC_TUNE_SHTCCTCTLPROT) |                  \
	 (DT_PROP(node, st_enable_hs_tx_staggering) ? STM32_USBPHYC_TUNE_STAGSEL : 0U))

#define USBPHYC_PORT_CONFIG(node)                                                                  \
	{                                                                                          \
		.index = DT_REG_ADDR(node), .tune = USBPHYC_PORT_TUNE(node),                       \
	}

/*
 * PHY port 1 is routed to the USB host block when a host controller node
 * references it, otherwise it stays connected to the OTG controller.
 */
#define USBPHYC_PORT1_NODE DT_INST_CHILD(0, usb_phy_1)

#define USBPHYC_NODE_IS_HOST(node)                                                                 \
	(DT_NODE_HAS_COMPAT(node, generic_ohci) || DT_NODE_HAS_COMPAT(node, generic_ehci))

#define USBPHYC_PORT1_HOST_USER(node)                                                              \
	COND_CODE_1(DT_NODE_HAS_PROP(node, phys),                                                  \
		    ((DT_SAME_NODE(DT_PHANDLE_BY_IDX(node, phys, 0), USBPHYC_PORT1_NODE) &&        \
		      USBPHYC_NODE_IS_HOST(node)) ||), (false ||))

#define USBPHYC_PORT1_HOST                                                                         \
	COND_CODE_1(DT_NODE_EXISTS(USBPHYC_PORT1_NODE),                                            \
		    (DT_FOREACH_STATUS_OKAY_NODE(USBPHYC_PORT1_HOST_USER) false), (false))

static const struct stm32_pclken usbphyc_pclken[] = STM32_DT_INST_CLOCKS(0);

static const struct usbphyc_port_config usbphyc_ports[] = {
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, USBPHYC_PORT_CONFIG, (,))
};

static const struct usbphyc_config usbphyc_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.pclken = usbphyc_pclken,
	.pclken_len = ARRAY_SIZE(usbphyc_pclken),
	.reset = RESET_DT_SPEC_INST_GET(0),
	.ports = usbphyc_ports,
	.num_ports = ARRAY_SIZE(usbphyc_ports),
	.port1_host = USBPHYC_PORT1_HOST,
};

/*
 * USB controllers reference a port with their phys property, the devicetree
 * dependency ordering initializes the PHY before the controllers.
 */
DEVICE_DT_INST_DEFINE(0, usbphyc_init, NULL, NULL, &usbphyc_cfg, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
