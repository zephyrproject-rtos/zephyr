/*
 * Copyright (c) 2023 Silicom Connectivity Solutions, Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pcr_rctl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>

#include <zephyr/logging/log.h>
#define XEC_RESET_SET_OFFSET(id) (((id) >> 5U) & 0xFFFU)
#define XEC_RESET_REG_BIT(id)	   ((id)&0x1FU)
#define XEC_RESET_LOCK_OFFSET	0x84
#define XEC_RESET_UNLOCK_VAL	0xA6382D4C
#define XEC_RESET_LOCK_VAL	0xA6382D4D

LOG_MODULE_REGISTER(reset_xec, 4);

struct reset_xec_config {
	uintptr_t base;
};

static int reset_xec_status(const struct device *dev, uint32_t id,
			      uint8_t *status)
{
	return -ENOSYS;
}

static int reset_xec_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_xec_config *config = dev->config;

	sys_write32(XEC_RESET_UNLOCK_VAL,
		    config->base + XEC_RESET_LOCK_OFFSET);
	sys_write32(BIT(XEC_RESET_REG_BIT(id)),
		    config->base + XEC_RESET_SET_OFFSET(id));
	sys_write32(XEC_RESET_LOCK_VAL,
		    config->base + XEC_RESET_LOCK_OFFSET);

	return 0;
}

static int reset_xec_line_deassert(const struct device *dev, uint32_t id)
{
	return -ENOSYS;
}

static int reset_xec_line_toggle(const struct device *dev, uint32_t id)
{
	reset_xec_line_assert(dev, id);

	return 0;
}

static int reset_xec_init(const struct device *dev)
{
	return 0;
}

static const struct reset_driver_api reset_xec_driver_api = {
	.status = reset_xec_status,
	.line_assert = reset_xec_line_assert,
	.line_deassert = reset_xec_line_deassert,
	.line_toggle = reset_xec_line_toggle,
};

static const struct reset_xec_config reset_xec_config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, reset_xec_init, NULL, NULL, &reset_xec_config,
		      PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
		      &reset_xec_driver_api);
