/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_pmuc_clk

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/clock/sf32lb-pmuc-clocks.h>
#include <register.h>

#define PMUC_LRC10_CR offsetof(PMUC_TypeDef, LRC10_CR)
#define PMUC_LRC32_CR offsetof(PMUC_TypeDef, LRC32_CR)

/* LRC10 and LRC32 nominal frequencies.
 * TODO: RC10k need measure actual frequencies and adjust these values if necessary.
 */
#define SF32LB_PMUC_LRC10_FREQ 10000U
#define SF32LB_PMUC_LRC32_FREQ 32000U

struct sf32lb_pmuc_clk_config {
	uintptr_t base;
};

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
	ARG_UNUSED(dev);

	return 0;
}

#define SF32LB_PMUC_CLK_CONFIG(inst)                                                               \
	static const struct sf32lb_pmuc_clk_config sf32lb_pmuc_clk_config_##inst = {               \
		.base = DT_REG_ADDR(DT_DRV_INST(inst)),                                            \
	}

#define SF32LB_PMUC_CLK_INIT(inst)                                                                 \
	SF32LB_PMUC_CLK_CONFIG(inst);                                                              \
	DEVICE_DT_INST_DEFINE(inst, sf32lb_pmuc_clk_init, NULL, NULL,                              \
			      &sf32lb_pmuc_clk_config_##inst, PRE_KERNEL_1,                        \
			      CONFIG_CLOCK_CONTROL_SF32LB_PMUC_INIT_PRIORITY,                      \
			      &sf32lb_pmuc_clk_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_PMUC_CLK_INIT)
