/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rstctl

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>

#include <fsl_device_registers.h>

#define NXP_RSTCTL_OFFSET(id) ((id >> 16) * sizeof(uint32_t))
#define NXP_RSTCTL_BIT(id) (BIT(id & 0xFFFF))
#define NXP_RSTCTL_CTL(id) (NXP_RSTCTL_OFFSET(id) + 0x10)
#define NXP_RSTCTL_SET(id) (NXP_RSTCTL_OFFSET(id) + 0x40)
#define NXP_RSTCTL_CLR(id) (NXP_RSTCTL_OFFSET(id) + 0x70)

struct reset_nxp_rstctl_config {
	volatile uint32_t *base;
	uint16_t offset_base;
};

static uint32_t reset_nxp_rstctl_normalize_id(const struct device *dev, uint32_t id)
{
	const struct reset_nxp_rstctl_config *config = dev->config;
	uint32_t offset = id >> 16;

	if ((config->offset_base != 0U) && (offset >= config->offset_base)) {
		offset -= config->offset_base;
		id = (offset << 16) | (id & 0xFFFF);
	}

	return id;
}

static int reset_nxp_rstctl_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_nxp_rstctl_config *config = dev->config;
	volatile uint32_t *base = config->base;

	id = reset_nxp_rstctl_normalize_id(dev, id);
	volatile const uint32_t *ctl_reg = base + (NXP_RSTCTL_CTL(id) / sizeof(uint32_t));
	uint32_t val = *ctl_reg;

	*status = (uint8_t)FIELD_GET(NXP_RSTCTL_BIT(id), val);

	return 0;
}

static int reset_nxp_rstctl_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_nxp_rstctl_config *config = dev->config;
	volatile uint32_t *base = config->base;

	id = reset_nxp_rstctl_normalize_id(dev, id);
	volatile uint32_t *set_reg = base + (NXP_RSTCTL_SET(id) / sizeof(uint32_t));

	*set_reg = FIELD_PREP(NXP_RSTCTL_BIT(id), 0b1);

	return 0;
}

static int reset_nxp_rstctl_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_nxp_rstctl_config *config = dev->config;
	volatile uint32_t *base = config->base;

	id = reset_nxp_rstctl_normalize_id(dev, id);
	volatile uint32_t *clr_reg = base + (NXP_RSTCTL_CLR(id) / sizeof(uint32_t));

	*clr_reg = FIELD_PREP(NXP_RSTCTL_BIT(id), 0b1);

	return 0;
}

static int reset_nxp_rstctl_line_toggle(const struct device *dev, uint32_t id)
{
	uint8_t status = 0;

	reset_nxp_rstctl_line_assert(dev, id);

	do {
		reset_nxp_rstctl_status(dev, id, &status);
	} while (status != 0b1);

	reset_nxp_rstctl_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_nxp_rstctl_driver_api) = {
	.status = reset_nxp_rstctl_status,
	.line_assert = reset_nxp_rstctl_line_assert,
	.line_deassert = reset_nxp_rstctl_line_deassert,
	.line_toggle = reset_nxp_rstctl_line_toggle,
};

#define NXP_RSTCTL_INIT(n)								\
	static const struct reset_nxp_rstctl_config reset_nxp_rstctl_config_##n = {	\
		.base = (volatile uint32_t *)DT_INST_REG_ADDR(n),			\
		.offset_base = DT_INST_PROP_OR(n, offset_base, 0),			\
	};										\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL,					\
			      &reset_nxp_rstctl_config_##n,				\
			      PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,			\
			      &reset_nxp_rstctl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_RSTCTL_INIT)
