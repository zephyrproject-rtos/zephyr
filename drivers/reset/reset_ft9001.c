/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT focaltech_ft9001_cpm_rctl

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/reset/focaltech_ft9001_reset.h>

/** Reset register offset (from encoded id) */
#define FT9001_RESET_REG_OFFSET(id) (((id) >> FOCALTECH_RESET_SHIFT) & 0xFFFFU)
/** Reset control bit (from encoded id) */
#define FT9001_RESET_BIT(id)        ((id) & FOCALTECH_RESET_MASK)

struct reset_ft9001_config {
	uint32_t base;
};

/**
 * @brief Get reset line status
 *
 * @param dev Reset controller device
 * @param id Reset line ID
 * @param status Pointer to store reset line status (0=deasserted, 1=asserted)
 *
 * @retval 0 Always successful
 */
static int reset_ft9001_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_ft9001_config *config = dev->config;
	uint32_t reg_offset = FT9001_RESET_REG_OFFSET(id);
	uint32_t bit = FT9001_RESET_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	*status = !!(*reg & BIT(bit));

	return 0;
}

/**
 * @brief Assert a reset line
 *
 * @param dev Reset controller device
 * @param id Reset line ID
 *
 * @retval 0 Always successful
 */
static int reset_ft9001_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_ft9001_config *config = dev->config;
	uint32_t reg_offset = FT9001_RESET_REG_OFFSET(id);
	uint32_t bit = FT9001_RESET_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	*reg |= BIT(bit);

	return 0;
}

/**
 * @brief Deassert a reset line
 *
 * @param dev Reset controller device
 * @param id Reset line ID
 *
 * @retval 0 Always successful
 */
static int reset_ft9001_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_ft9001_config *config = dev->config;
	uint32_t reg_offset = FT9001_RESET_REG_OFFSET(id);
	uint32_t bit = FT9001_RESET_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	*reg &= ~BIT(bit);

	return 0;
}

/**
 * @brief Toggle a reset line (assert then deassert)
 *
 * @param dev Reset controller device
 * @param id Reset line ID
 *
 * @retval 0 Always successful
 */
static int reset_ft9001_line_toggle(const struct device *dev, uint32_t id)
{
	reset_ft9001_line_assert(dev, id);
	reset_ft9001_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_ft9001_driver_api) = {
	.status = reset_ft9001_status,
	.line_assert = reset_ft9001_line_assert,
	.line_deassert = reset_ft9001_line_deassert,
	.line_toggle = reset_ft9001_line_toggle,
};

static const struct reset_ft9001_config ft9001_reset_config = {
	.base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(0))),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &ft9001_reset_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_ft9001_driver_api);
