/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_pmuc_clk

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/clock/sf32lb-pmuc-clocks.h>
#include <register.h>

#define PMUC_CR       offsetof(PMUC_TypeDef, CR)
#define PMUC_LRC10_CR offsetof(PMUC_TypeDef, LRC10_CR)
#define PMUC_LRC32_CR offsetof(PMUC_TypeDef, LRC32_CR)

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
	const struct pinctrl_dev_config *pinctrl;
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
	ARG_UNUSED(dev);

	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case SF32LB_PMUC_CLOCK_LRC10:
		*rate = SF32LB_PMUC_LRC10_FREQ;
		return 0;
	case SF32LB_PMUC_CLOCK_LRC32:
		*rate = SF32LB_PMUC_LRC32_FREQ;
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

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (config->clkwdt_src == SF32LB_CLKWDT_SRC_LRC32) {
		clk_id = SF32LB_PMUC_CLOCK_LRC32;
	}

	ret = sf32lb_pmuc_clk_on(dev, (clock_control_subsys_t)(uintptr_t)clk_id);
	if (ret < 0) {
		return ret;
	}

	sf32lb_pmuc_select_lpclk(config);

	return 0;
}
BUILD_ASSERT(SF32LB_CLOCK_NODE_ENABLED(0, lrc32) || SF32LB_CLOCK_NODE_ENABLED(0, lrc10),
	     "At least one low-speed RC oscillator must be enabled");

#define SF32LB_PMUC_CLK_CONFIG(inst)                                                               \
	static const struct sf32lb_pmuc_clk_config sf32lb_pmuc_clk_config_##inst = {               \
		.base = DT_REG_ADDR(DT_DRV_INST(inst)),                                            \
		.clkwdt_src = SF32LB_CLKWDT_SRC_IDX(inst),                                         \
	}

#define SF32LB_PMUC_CLK_INIT(inst)                                                                 \
	SF32LB_PMUC_CLK_CONFIG(inst);                                                              \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	DEVICE_DT_INST_DEFINE(inst, sf32lb_pmuc_clk_init, NULL, NULL,                              \
			      &sf32lb_pmuc_clk_config_##inst, PRE_KERNEL_1,                        \
			      CONFIG_CLOCK_CONTROL_SF32LB_PMUC_INIT_PRIORITY,                      \
			      &sf32lb_pmuc_clk_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_PMUC_CLK_INIT)
