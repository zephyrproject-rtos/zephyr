/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_pmuc_clk

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/clock/sf32lb-pmuc-clocks.h>
#include <zephyr/dt-bindings/pinctrl/sf32lb52x-pinctrl.h>
#include <pinctrl_soc.h>
#include <register.h>

#define PMUC_CR       offsetof(PMUC_TypeDef, CR)
#define PMUC_LRC10_CR offsetof(PMUC_TypeDef, LRC10_CR)
#define PMUC_LRC32_CR offsetof(PMUC_TypeDef, LRC32_CR)
#define PMUC_LXT_CR   offsetof(PMUC_TypeDef, LXT_CR)

#define PMUC_LXT_CR_EN        PMUC_LXT_CR_EN_Msk
#define PMUC_LXT_BM_VALUE     0x2U
#define PMUC_LXT_AMP_BM_VALUE 0x3U

#if DT_HAS_COMPAT_STATUS_OKAY(sifli_sf32lb_lxt32)
#define SF32LB_LXT32_NODE    DT_NODELABEL(lxt32)
#define SF32LB_LXT32_FREQ_HZ DT_PROP(SF32LB_LXT32_NODE, clock_frequency)
#define SF32LB_PMUC_HAS_LXT32(inst)                                                                \
	DT_SAME_NODE(DT_PHANDLE(SF32LB_LXT32_NODE, sifli_pmuc), DT_INST_PARENT(inst))
#else
#define SF32LB_LXT32_FREQ_HZ        0U
#define SF32LB_PMUC_HAS_LXT32(inst) 0
#endif

enum sf32lb_clkwdt_src_idx {
	SF32LB_CLKWDT_SRC_LRC10 = 0,
	SF32LB_CLKWDT_SRC_LRC32 = 1,
};

#define SF32LB_CLKWDT_SRC_IDX(inst)                                                                \
	DT_ENUM_IDX_OR(DT_DRV_INST(inst), sifli_clk_wdt_src, SF32LB_CLKWDT_SRC_LRC10)

/* LRC10 and LRC32 nominal frequencies.
 * TODO: RC10k need measure actual frequencies and adjust these values if necessary.
 */
#define SF32LB_PMUC_LRC10_FREQ 10000U
#define SF32LB_PMUC_LRC32_FREQ 32000U

struct sf32lb_pmuc_clk_config {
	uintptr_t base;
	uint8_t clkwdt_src;
	uint8_t has_lxt32;
	uint32_t lxt32_freq;
	uintptr_t pad_pa;
};

static void sf32lb_pmuc_select_lpclk(const struct sf32lb_pmuc_clk_config *config)
{
	uint32_t val = sys_read32(config->base + PMUC_CR);

	if (config->clkwdt_src == SF32LB_CLKWDT_SRC_LRC32) {
		val |= PMUC_CR_SEL_LPCLK;
	} else {
		val &= ~PMUC_CR_SEL_LPCLK;
	}

	sys_write32(val, config->base + PMUC_CR);
}

static void sf32lb_pmuc_configure_lxt32_pin(uintptr_t pad_pa, uint32_t pinmux)
{
	uint32_t val;
	uintptr_t pad;

	pad = pad_pa + (FIELD_GET(SF32LB_PAD_MSK, pinmux) * 4U);
	val = sys_read32(pad);
	val &= ~SF32LB_PINMUX_CFG_MSK;
	val |= (pinmux & (SF32LB_PINMUX_CFG_MSK & ~SF32LB_DS_MSK));
	sys_write32(val, pad);
}

static void sf32lb_pmuc_configure_lxt32_pins(const struct sf32lb_pmuc_clk_config *config)
{
	/* LXT32 pins are fixed on PA22/PA23 for SF32LB52X. */
	sf32lb_pmuc_configure_lxt32_pin(config->pad_pa, PA22_XTAL32K_XI);
	sf32lb_pmuc_configure_lxt32_pin(config->pad_pa, PA23_XTAL32K_XO);
}

static int sf32lb_pmuc_lxt32_on(const struct sf32lb_pmuc_clk_config *config)
{
	uint32_t val;

	/* Configure bias current and enable in a single write */
	val = sys_read32(config->base + PMUC_LXT_CR);
	val &= ~(PMUC_LXT_CR_EN | PMUC_LXT_CR_RSN | PMUC_LXT_CR_CAP_SEL | PMUC_LXT_CR_BM_Msk |
		 PMUC_LXT_CR_AMP_BM_Msk);
	val |= FIELD_PREP(PMUC_LXT_CR_BM_Msk, PMUC_LXT_BM_VALUE) |
	       FIELD_PREP(PMUC_LXT_CR_AMP_BM_Msk, PMUC_LXT_AMP_BM_VALUE) | PMUC_LXT_CR_EN |
	       PMUC_LXT_CR_RSN;
	sys_write32(val, config->base + PMUC_LXT_CR);

	while (sys_test_bit(config->base + PMUC_LXT_CR, PMUC_LXT_CR_RDY_Pos) == 0U) {
	}

	return 0;
}

static int sf32lb_pmuc_lxt32_off(const struct sf32lb_pmuc_clk_config *config)
{
	sys_clear_bits(config->base + PMUC_LXT_CR, PMUC_LXT_CR_EN | PMUC_LXT_CR_RSN);

	return 0;
}

static int sf32lb_pmuc_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct sf32lb_pmuc_clk_config *config = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case SF32LB_PMUC_CLOCK_LRC10:
		sys_set_bit(config->base + PMUC_LRC10_CR, PMUC_LRC10_CR_EN_Pos);
		while (sys_test_bit(config->base + PMUC_LRC10_CR, PMUC_LRC10_CR_RDY_Pos) == 0U) {
		}
		return 0;
	case SF32LB_PMUC_CLOCK_LRC32:
		sys_set_bit(config->base + PMUC_LRC32_CR, PMUC_LRC32_CR_EN_Pos);
		while (sys_test_bit(config->base + PMUC_LRC32_CR, PMUC_LRC32_CR_RDY_Pos) == 0U) {
		}
		return 0;
	case SF32LB_PMUC_CLOCK_LXT32:
		if (config->has_lxt32 == 0U) {
			return -ENOTSUP;
		}
		sf32lb_pmuc_configure_lxt32_pins(config);
		return sf32lb_pmuc_lxt32_on(config);
	default:
		break;
	}

	return -ENOTSUP;
}

static int sf32lb_pmuc_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct sf32lb_pmuc_clk_config *config = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case SF32LB_PMUC_CLOCK_LRC10:
		sys_clear_bit(config->base + PMUC_LRC10_CR, PMUC_LRC10_CR_EN_Pos);
		return 0;
	case SF32LB_PMUC_CLOCK_LRC32:
		sys_clear_bit(config->base + PMUC_LRC32_CR, PMUC_LRC32_CR_EN_Pos);
		return 0;
	case SF32LB_PMUC_CLOCK_LXT32:
		if (config->has_lxt32 == 0U) {
			return -ENOTSUP;
		}
		return sf32lb_pmuc_lxt32_off(config);
	default:
		break;
	}

	return -ENOTSUP;
}

static enum clock_control_status sf32lb_pmuc_clk_get_status(const struct device *dev,
							    clock_control_subsys_t sys)
{
	const struct sf32lb_pmuc_clk_config *config = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uintptr_t reg;
	uint32_t bit;

	switch (id) {
	case SF32LB_PMUC_CLOCK_LRC10:
		reg = config->base + PMUC_LRC10_CR;
		bit = PMUC_LRC10_CR_RDY_Pos;
		break;
	case SF32LB_PMUC_CLOCK_LRC32:
		reg = config->base + PMUC_LRC32_CR;
		bit = PMUC_LRC32_CR_RDY_Pos;
		break;
	case SF32LB_PMUC_CLOCK_LXT32:
		if (config->has_lxt32 == 0U) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		reg = config->base + PMUC_LXT_CR;
		bit = PMUC_LXT_CR_RDY_Pos;
		break;
	default:
		return CLOCK_CONTROL_STATUS_OFF;
	}

	if (sys_test_bit(reg, bit) != 0U) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int sf32lb_pmuc_clk_get_rate(const struct device *dev, clock_control_subsys_t sys,
				    uint32_t *rate)
{
	const struct sf32lb_pmuc_clk_config *config = dev->config;

	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case SF32LB_PMUC_CLOCK_LRC10:
		*rate = SF32LB_PMUC_LRC10_FREQ;
		return 0;
	case SF32LB_PMUC_CLOCK_LRC32:
		*rate = SF32LB_PMUC_LRC32_FREQ;
		return 0;
	case SF32LB_PMUC_CLOCK_LXT32:
		if (config->has_lxt32 == 0U) {
			return -ENOTSUP;
		}
		*rate = config->lxt32_freq;
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static DEVICE_API(clock_control, sf32lb_pmuc_clk_api) = {
	.on = sf32lb_pmuc_clk_on,
	.off = sf32lb_pmuc_clk_off,
	.get_rate = sf32lb_pmuc_clk_get_rate,
	.get_status = sf32lb_pmuc_clk_get_status,
};

static int sf32lb_pmuc_clk_init(const struct device *dev)
{
	const struct sf32lb_pmuc_clk_config *config = dev->config;
	uint32_t clk_id = SF32LB_PMUC_CLOCK_LRC10;
	int ret;

	if (config->clkwdt_src == SF32LB_CLKWDT_SRC_LRC32) {
		clk_id = SF32LB_PMUC_CLOCK_LRC32;
	}

	ret = sf32lb_pmuc_clk_on(dev, (clock_control_subsys_t)(uintptr_t)clk_id);
	if (ret < 0) {
		return ret;
	}

	sf32lb_pmuc_select_lpclk(config);

	if (config->has_lxt32 != 0U) {
		ret = sf32lb_pmuc_clk_on(
			dev, (clock_control_subsys_t)(uintptr_t)SF32LB_PMUC_CLOCK_LXT32);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define SF32LB_PMUC_CLK_CONFIG(inst)                                                               \
	static const struct sf32lb_pmuc_clk_config sf32lb_pmuc_clk_config_##inst = {               \
		.base = DT_REG_ADDR(DT_DRV_INST(inst)),                                            \
		.clkwdt_src = SF32LB_CLKWDT_SRC_IDX(inst),                                         \
		.has_lxt32 = SF32LB_PMUC_HAS_LXT32(inst),                                          \
		.lxt32_freq = SF32LB_LXT32_FREQ_HZ,                                                \
		.pad_pa = DT_REG_ADDR_BY_NAME(DT_NODELABEL(pinctrl), pad_pa),                      \
	}

#define SF32LB_PMUC_CLK_INIT(inst)                                                                 \
	SF32LB_PMUC_CLK_CONFIG(inst);                                                              \
	DEVICE_DT_INST_DEFINE(inst, sf32lb_pmuc_clk_init, NULL, NULL,                              \
			      &sf32lb_pmuc_clk_config_##inst, PRE_KERNEL_1,                        \
			      CONFIG_CLOCK_CONTROL_SF32LB_PMUC_INIT_PRIORITY,                      \
			      &sf32lb_pmuc_clk_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_PMUC_CLK_INIT)
