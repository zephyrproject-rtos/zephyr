/*
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_pmc

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static int atmel_sam_clock_control_on(const struct device *dev,
				      clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	const struct atmel_sam_pmc_config *cfg = (const struct atmel_sam_pmc_config *)sys;

	if (cfg == NULL) {
		LOG_ERR("The PMC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("Type: %x, Id: %d", cfg->clock_type, cfg->peripheral_id);

	switch (cfg->clock_type) {
	case PMC_TYPE_PERIPHERAL:
		soc_pmc_peripheral_enable(cfg->peripheral_id);
		break;
	default:
		LOG_ERR("The PMC clock type is not implemented.");
		return -ENODEV;
	}

	return 0;
}

static int atmel_sam_clock_control_off(const struct device *dev,
				       clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	const struct atmel_sam_pmc_config *cfg = (const struct atmel_sam_pmc_config *)sys;

	if (cfg == NULL) {
		LOG_ERR("The PMC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("Type: %x, Id: %d", cfg->clock_type, cfg->peripheral_id);

	switch (cfg->clock_type) {
	case PMC_TYPE_PERIPHERAL:
		soc_pmc_peripheral_disable(cfg->peripheral_id);
		break;
	default:
		LOG_ERR("The PMC clock type is not implemented.");
		return -ENODEV;
	}

	return 0;
}

static int atmel_sam_clock_control_get_rate(const struct device *dev,
					    clock_control_subsys_t sys,
					    uint32_t *rate)
{
	ARG_UNUSED(dev);

	const struct atmel_sam_pmc_config *cfg = (const struct atmel_sam_pmc_config *)sys;

	if (cfg == NULL) {
		LOG_ERR("The PMC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("Type: %x, Id: %d", cfg->clock_type, cfg->peripheral_id);

	switch (cfg->clock_type) {
	case PMC_TYPE_PERIPHERAL:
		*rate = SOC_ATMEL_SAM_MCK_FREQ_HZ;
		break;
	default:
		LOG_ERR("The PMC clock type is not implemented.");
		return -ENODEV;
	}

	LOG_DBG("Rate: %d", *rate);

	return 0;
}

static enum clock_control_status
atmel_sam_clock_control_get_status(const struct device *dev,
				   clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	const struct atmel_sam_pmc_config *cfg = (const struct atmel_sam_pmc_config *)sys;
	enum clock_control_status status;

	if (cfg == NULL) {
		LOG_ERR("The PMC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("Type: %x, Id: %d", cfg->clock_type, cfg->peripheral_id);

	switch (cfg->clock_type) {
	case PMC_TYPE_PERIPHERAL:
		status = soc_pmc_peripheral_is_enabled(cfg->peripheral_id) > 0
		       ? CLOCK_CONTROL_STATUS_ON
		       : CLOCK_CONTROL_STATUS_OFF;
		break;
	default:
		LOG_ERR("The PMC clock type is not implemented.");
		return -ENODEV;
	}

	return status;
}

static DEVICE_API(clock_control, atmel_sam_clock_control_api) = {
	.on = atmel_sam_clock_control_on,
	.off = atmel_sam_clock_control_off,
	.get_rate = atmel_sam_clock_control_get_rate,
	.get_status = atmel_sam_clock_control_get_status,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &atmel_sam_clock_control_api);
