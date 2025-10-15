/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch, ch32_rcc_rctl

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/dt-bindings/reset/ch32-common.h>
#include <zephyr/sys/__assert.h>

#include <hal_ch32fun.h>

#define CH32_CHECK_ADDR(id)                                                                        \
	__ASSERT(CH32_RESET_BUS(id) <= (DT_INST_REG_SIZE(0) / 4),                                  \
		 "Reset ID is outside of assert registers")

struct reset_ch32_config {
	mem_addr_t base;
};

static int reset_ch32_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_ch32_config *config = dev->config;

	CH32_CHECK_ADDR(id);

	*status = !!sys_test_bit(config->base + CH32_RESET_BUS(id), CH32_RESET_BIT(id));

	return 0;
}

static int reset_ch32_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_ch32_config *config = dev->config;

	CH32_CHECK_ADDR(id);

	sys_set_bit(config->base + CH32_RESET_BUS(id), CH32_RESET_BIT(id));

	return 0;
}

static int reset_ch32_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_ch32_config *config = dev->config;

	CH32_CHECK_ADDR(id);

	sys_clear_bit(config->base + CH32_RESET_BUS(id), CH32_RESET_BIT(id));

	return 0;
}

static int reset_ch32_line_toggle(const struct device *dev, uint32_t id)
{
	reset_ch32_line_assert(dev, id);
	reset_ch32_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_ch32_driver_api) = {
	.status = reset_ch32_status,
	.line_assert = reset_ch32_line_assert,
	.line_deassert = reset_ch32_line_deassert,
	.line_toggle = reset_ch32_line_toggle,
};

static const struct reset_ch32_config reset_ch32_config = {
	.base = (mem_addr_t)(DT_REG_ADDR_BY_NAME(DT_INST_PARENT(0), rcc) + DT_INST_REG_ADDR(0)),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_ch32_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_ch32_driver_api);
