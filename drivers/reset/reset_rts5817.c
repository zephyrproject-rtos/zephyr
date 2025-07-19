/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_reset

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include "reset_rts5817.h"

struct reset_rts5817_config {
	uintptr_t base;
};

static int reset_rts5817_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_rts5817_config *config = dev->config;

	*status = !!sys_test_bit(config->base + R_SYS_FORCE_RST, id);

	return 0;
}

static int reset_rts5817_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_rts5817_config *config = dev->config;

	sys_set_bit(config->base + R_SYS_FORCE_RST, id);

	return 0;
}

static int reset_rts5817_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_rts5817_config *config = dev->config;

	sys_clear_bit(config->base + R_SYS_FORCE_RST, id);

	return 0;
}

static int reset_rts5817_line_toggle(const struct device *dev, uint32_t id)
{
	reset_rts5817_line_assert(dev, id);
	reset_rts5817_line_deassert(dev, id);

	return 0;
}

static const struct reset_driver_api reset_rts5817_driver_api = {
	.status = reset_rts5817_status,
	.line_assert = reset_rts5817_line_assert,
	.line_deassert = reset_rts5817_line_deassert,
	.line_toggle = reset_rts5817_line_toggle,
};

static const struct reset_rts5817_config reset_rts5817_config = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_rts5817_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_rts5817_driver_api);
