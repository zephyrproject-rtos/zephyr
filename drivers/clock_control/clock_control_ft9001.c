/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT focaltech_ft9001_cpm

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/focaltech_ft9001_clocks.h>

/** Clock register offset (from encoded id) */
#define FT9001_CLOCK_REG_OFFSET(id) (((id) >> FOCALTECH_CLOCK_SHIFT) & 0xFFFFU)
/** Clock control bit (from encoded id) */
#define FT9001_CLOCK_BIT(id)        ((id) & FOCALTECH_CLOCK_MASK)

struct clock_control_ft9001_config {
	uint32_t base;
};

/**
 * @brief Enable a clock
 *
 * @param dev Clock control device
 * @param sys Clock subsystem ID
 *
 * @retval 0 Always successful
 */
static int clock_control_ft9001_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_ft9001_config *config = dev->config;
	uint32_t id = POINTER_TO_UINT(sys);
	uint32_t reg_offset = FT9001_CLOCK_REG_OFFSET(id);
	uint32_t bit = FT9001_CLOCK_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	*reg |= BIT(bit);

	return 0;
}

/**
 * @brief Disable a clock
 *
 * @param dev Clock control device
 * @param sys Clock subsystem ID
 *
 * @retval 0 Always successful
 */
static int clock_control_ft9001_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_ft9001_config *config = dev->config;
	uint32_t id = POINTER_TO_UINT(sys);
	uint32_t reg_offset = FT9001_CLOCK_REG_OFFSET(id);
	uint32_t bit = FT9001_CLOCK_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	*reg &= ~BIT(bit);

	return 0;
}

/**
 * @brief Get clock status
 *
 * @param dev Clock control device
 * @param sys Clock subsystem ID
 *
 * @retval CLOCK_CONTROL_STATUS_ON if clock is enabled
 * @retval CLOCK_CONTROL_STATUS_OFF if clock is disabled
 */
static enum clock_control_status clock_control_ft9001_get_status(const struct device *dev,
								 clock_control_subsys_t sys)
{
	const struct clock_control_ft9001_config *config = dev->config;
	uint32_t id = POINTER_TO_UINT(sys);
	uint32_t reg_offset = FT9001_CLOCK_REG_OFFSET(id);
	uint32_t bit = FT9001_CLOCK_BIT(id);
	volatile uint32_t *reg = (volatile uint32_t *)(config->base + reg_offset);

	if ((*reg & BIT(bit)) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, clock_control_ft9001_api) = {
	.on = clock_control_ft9001_on,
	.off = clock_control_ft9001_off,
	.get_status = clock_control_ft9001_get_status,
};

static const struct clock_control_ft9001_config ft9001_cpm_config = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &ft9001_cpm_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_ft9001_api);
