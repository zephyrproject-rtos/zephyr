/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rcc_rctl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>

#define SF32LB_RESET_OFFSET(id) ((((id) >> 5U) & 0x1U) << 2U)

struct sf32lb_reset_config {
	uintptr_t base;
};

static int sf32lb_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct sf32lb_reset_config *config = dev->config;

	*status = !!sys_test_bit(config->base + SF32LB_RESET_OFFSET(id), id);

	return 0;
}

static int sf32lb_reset_line_assert(const struct device *dev, uint32_t id)
{
	const struct sf32lb_reset_config *config = dev->config;

	sys_set_bit(config->base + SF32LB_RESET_OFFSET(id), id);

	return 0;
}

static int sf32lb_reset_line_deassert(const struct device *dev, uint32_t id)
{
	const struct sf32lb_reset_config *config = dev->config;

	sys_clear_bit(config->base + SF32LB_RESET_OFFSET(id), id);

	return 0;
}

static int sf32lb_reset_line_toggle(const struct device *dev, uint32_t id)
{
	sf32lb_reset_line_assert(dev, id);
	sf32lb_reset_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, sf32lb_reset_api) = {
	.status = sf32lb_reset_status,
	.line_assert = sf32lb_reset_line_assert,
	.line_deassert = sf32lb_reset_line_deassert,
	.line_toggle = sf32lb_reset_line_toggle,
};

static const struct sf32lb_reset_config sf32lb_reset_cfg = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &sf32lb_reset_cfg, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &sf32lb_reset_api);
