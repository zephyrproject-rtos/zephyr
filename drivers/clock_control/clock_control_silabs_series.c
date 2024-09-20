/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_series_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "sl_clock_manager.h"
#include "sl_status.h"

struct silabs_clock_control_config {
	CMU_TypeDef *cmu;
};

static enum clock_control_status silabs_clock_control_get_status(const struct device *dev,
								 clock_control_subsys_t sys);

static int silabs_clock_control_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct silabs_clock_control_cmu_config *cfg = sys;
	sl_status_t status;

	if (silabs_clock_control_get_status(dev, sys) == CLOCK_CONTROL_STATUS_ON) {
		return -EALREADY;
	}

	status = sl_clock_manager_enable_bus_clock(&cfg->bus_clock);
	if (status != SL_STATUS_OK) {
		return -ENOTSUP;
	}

	return 0;
}

static int silabs_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct silabs_clock_control_cmu_config *cfg = sys;
	sl_status_t status;

	status = sl_clock_manager_disable_bus_clock(&cfg->bus_clock);
	if (status != SL_STATUS_OK) {
		return -ENOTSUP;
	}

	return 0;
}

static int silabs_clock_control_get_rate(const struct device *dev, clock_control_subsys_t sys,
					 uint32_t *rate)
{
	const struct silabs_clock_control_cmu_config *cfg = sys;
	sl_status_t status;

	status = sl_clock_manager_get_clock_branch_frequency(cfg->branch, rate);
	if (status != SL_STATUS_OK) {
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status silabs_clock_control_get_status(const struct device *dev,
								 clock_control_subsys_t sys)
{
	const struct silabs_clock_control_cmu_config *cfg = sys;
	__maybe_unused const struct silabs_clock_control_config *reg = dev->config;
	uint32_t clock_status = 0;

	if (cfg->bus_clock == 0xFFFFFFFFUL) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	switch (FIELD_GET(CLOCK_REG_MASK, cfg->bus_clock)) {
#if defined(_CMU_CLKEN0_MASK)
	case 0:
		clock_status = reg->cmu->CLKEN0;
		break;
#endif
#if defined(_CMU_CLKEN1_MASK)
	case 1:
		clock_status = reg->cmu->CLKEN1;
		break;
#endif
#if defined(_CMU_CLKEN2_MASK)
	case 2:
		clock_status = reg->cmu->CLKEN2;
		break;
#endif
	default:
		__ASSERT(false, "Invalid bus clock: %x\n", cfg->bus_clock);
		break;
	}

	if (clock_status & BIT(FIELD_GET(CLOCK_BIT_MASK, cfg->bus_clock))) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int silabs_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	sl_clock_manager_runtime_init();

	return 0;
}

static const struct clock_control_driver_api silabs_clock_control_api = {
	.on = silabs_clock_control_on,
	.off = silabs_clock_control_off,
	.get_rate = silabs_clock_control_get_rate,
	.get_status = silabs_clock_control_get_status,
};

static const struct silabs_clock_control_config silabs_clock_control_config = {
	.cmu = (CMU_TypeDef *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, silabs_clock_control_init, NULL, NULL, &silabs_clock_control_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &silabs_clock_control_api);
