/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32_rcc_rctl

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/arch/common/sys_bitops.h>

#define CH32_RESET_REG_OFFEST(id) (((id) >> 5U) & 0xFFU)
#define CH32_RESET_REG_BIT(id)    ((id) & 0x1FU)

struct reset_ch32_config {
	mem_addr_t base;
};

static int reset_ch32_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_ch32_config *const config = dev->config;

	*status = !!sys_test_bit(config->base + CH32_RESET_REG_OFFEST(id), CH32_RESET_REG_BIT(id));

	return 0;
}

static int reset_ch32_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_ch32_config *const config = dev->config;

	sys_set_bit(config->base + CH32_RESET_REG_OFFEST(id), CH32_RESET_REG_BIT(id));

	return 0;
}

static int reset_ch32_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_ch32_config *const config = dev->config;

	sys_clear_bit(config->base + CH32_RESET_REG_OFFEST(id), CH32_RESET_REG_BIT(id));

	return 0;
}

static int reset_ch32_line_toggle(const struct device *dev, uint32_t id)
{
	reset_ch32_line_assert(dev, id);
	reset_ch32_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_ch32_drive_api) = {
	.status = reset_ch32_status,
	.line_assert = reset_ch32_line_assert,
	.line_deassert = reset_ch32_line_deassert,
	.line_toggle = reset_ch32_line_toggle,
};

static const struct reset_ch32_config reset_ch32_config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_ch32_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_ch32_drive_api);
