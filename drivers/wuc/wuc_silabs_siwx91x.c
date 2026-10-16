/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_wuc

#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/silabs-siwx91x-wuc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <rsi_power_save.h>
#include <sl_si91x_power_manager.h>

LOG_MODULE_REGISTER(siwx91x_wuc, CONFIG_WUC_LOG_LEVEL);

static int siwx91x_wuc_source_to_mask(uint32_t id, uint32_t *mask)
{
	if (mask == NULL) {
		return -EINVAL;
	}

	switch (id) {
	case SIWX91X_WUC_RTC_ALARM:
		*mask = SL_SI91X_POWER_MANAGER_ALARM_WAKEUP;
		return 0;
	default:
		return -EINVAL;
	}
}

static int siwx91x_wuc_enable(const struct device *dev, uint32_t id)
{
	sl_status_t status;
	uint32_t mask;
	int ret;

	ARG_UNUSED(dev);

	ret = siwx91x_wuc_source_to_mask(id, &mask);
	if (ret != 0) {
		return ret;
	}

	status = sl_si91x_power_manager_set_wakeup_sources(mask, true);
	return status == SL_STATUS_OK ? 0 : -EIO;
}

static int siwx91x_wuc_disable(const struct device *dev, uint32_t id)
{
	sl_status_t status;
	uint32_t mask;
	int ret;

	ARG_UNUSED(dev);

	ret = siwx91x_wuc_source_to_mask(id, &mask);
	if (ret != 0) {
		return ret;
	}

	status = sl_si91x_power_manager_set_wakeup_sources(mask, false);
	return status == SL_STATUS_OK ? 0 : -EIO;
}

static int siwx91x_wuc_triggered(const struct device *dev, uint32_t id)
{
	ARG_UNUSED(dev);

	if (id != SIWX91X_WUC_RTC_ALARM) {
		return -EINVAL;
	}

	return (MCU_FSM->MCU_FSM_WAKEUP_STATUS_REG &
		RTC_ALARM_BASED_WAKEUP_STATUS_CLEAR) != 0U;
}

static int siwx91x_wuc_clear(const struct device *dev, uint32_t id)
{
	ARG_UNUSED(dev);

	if (id != SIWX91X_WUC_RTC_ALARM) {
		return -EINVAL;
	}

	MCU_FSM->MCU_FSM_WAKEUP_STATUS_CLEAR =
		RTC_ALARM_BASED_WAKEUP_STATUS_CLEAR;

	return 0;
}

static DEVICE_API(wuc, siwx91x_wuc_api) = {
	.enable = siwx91x_wuc_enable,
	.disable = siwx91x_wuc_disable,
	.triggered = siwx91x_wuc_triggered,
	.clear = siwx91x_wuc_clear,
};

#define SIWX91X_WUC_INIT(inst)                                                                    \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                         \
			      CONFIG_WUC_INIT_PRIORITY, &siwx91x_wuc_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_WUC_INIT)
