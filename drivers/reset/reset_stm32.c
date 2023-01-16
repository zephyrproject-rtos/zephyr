/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_rcc_rctl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>

#define STM32_RESET_CLR_OFFSET(id) (((id) >> 17U) & 0xFFFU)
#define STM32_RESET_SET_OFFSET(id) (((id) >> 5U) & 0xFFFU)
#define STM32_RESET_REG_BIT(id)	   ((id)&0x1FU)

struct reset_stm32_config {
	uintptr_t base;
};

static int reset_stm32_status(const struct device *dev, uint32_t id,
			      uint8_t *status)
{
	const struct reset_stm32_config *config = dev->config;

	*status = !!sys_test_bit(config->base + STM32_RESET_SET_OFFSET(id),
				 STM32_RESET_REG_BIT(id));

	return 0;
}

static int reset_stm32_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_stm32_config *config = dev->config;

	sys_set_bit(config->base + STM32_RESET_SET_OFFSET(id),
		    STM32_RESET_REG_BIT(id));

	return 0;
}

static int reset_stm32_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_stm32_config *config = dev->config;

#if DT_INST_PROP(0, set_bit_to_deassert)
	sys_set_bit(config->base + STM32_RESET_CLR_OFFSET(id),
		    STM32_RESET_REG_BIT(id));
#else
	sys_clear_bit(config->base + STM32_RESET_SET_OFFSET(id),
		      STM32_RESET_REG_BIT(id));
#endif

	return 0;
}

static int reset_stm32_line_toggle(const struct device *dev, uint32_t id)
{
	reset_stm32_line_assert(dev, id);
	reset_stm32_line_deassert(dev, id);

	return 0;
}

static int reset_stm32_init(const struct device *dev)
{
	return 0;
}

static const struct reset_driver_api reset_stm32_driver_api = {
	.status = reset_stm32_status,
	.line_assert = reset_stm32_line_assert,
	.line_deassert = reset_stm32_line_deassert,
	.line_toggle = reset_stm32_line_toggle,
};

static const struct reset_stm32_config reset_stm32_config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, reset_stm32_init, NULL, NULL, &reset_stm32_config,
		      PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
		      &reset_stm32_driver_api);
