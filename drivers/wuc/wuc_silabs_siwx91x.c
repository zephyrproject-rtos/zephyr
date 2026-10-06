/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rsi_system_config.h"
#define DT_DRV_COMPAT silabs_siwx91x_wuc

#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/silabs-siwx91x-wuc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <rsi_power_save.h>
#include <sl_si91x_power_manager.h>

LOG_MODULE_REGISTER(siwx91x_wuc, CONFIG_WUC_LOG_LEVEL);

/* NPSS/UULP GPIO wake status (MCU_FSM holds RTC alarm status). */
#define SIWX91X_NPSS_GPIO_WAKE_STATUS_MASK                                                         \
	(NPSS_TO_MCU_GPIO_INTR_0 | NPSS_TO_MCU_GPIO_INTR_1 | NPSS_TO_MCU_GPIO_INTR_2 |             \
	 NPSS_TO_MCU_GPIO_INTR_3 | NPSS_TO_MCU_GPIO_INTR_4)

struct wakeup_stat {
	uint32_t mask;
	volatile uint32_t *status_reg;
	volatile uint32_t *clear_reg;
	uint32_t clear_mask;
};
static const struct wakeup_stat wakeup_sources[] = {
	[SIWX91X_WUC_RTC_ALARM] = {.mask = SL_SI91X_POWER_MANAGER_ALARM_WAKEUP,
				   .status_reg = &(MCU_FSM->MCU_FSM_WAKEUP_STATUS_REG),
				   .clear_reg = &(MCU_FSM->MCU_FSM_WAKEUP_STATUS_CLEAR),
				   .clear_mask = RTC_ALARM_BASED_WAKEUP_STATUS_CLEAR},
	[SIWX91X_WUC_GPIO] = {.mask = SL_SI91X_POWER_MANAGER_GPIO_WAKEUP,
			      .status_reg = &NPSS_INTR_STATUS_REG,
			      .clear_reg = &NPSS_INTR_CLEAR_REG,
			      .clear_mask = SIWX91X_NPSS_GPIO_WAKE_STATUS_MASK},
};
static int siwx91x_wuc_source_to_mask(uint32_t id, uint32_t *mask)
{
	if (mask == NULL || (id >= ARRAY_SIZE(wakeup_sources))) {
		return -EINVAL;
	}

	*mask = wakeup_sources[id].mask;
	return 0;
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
	if (id >= ARRAY_SIZE(wakeup_sources)) {
		return -EINVAL;
	}
	return (*wakeup_sources[id].status_reg & wakeup_sources[id].clear_mask) != 0U;
}

static int siwx91x_wuc_clear(const struct device *dev, uint32_t id)
{
	uint32_t pending;
	ARG_UNUSED(dev);
	if (id >= ARRAY_SIZE(wakeup_sources)) {
		return -EINVAL;
	}
	pending = *wakeup_sources[id].status_reg & wakeup_sources[id].clear_mask;
	*wakeup_sources[id].clear_reg = pending;
	return 0;
}

static DEVICE_API(wuc, siwx91x_wuc_api) = {
	.enable = siwx91x_wuc_enable,
	.disable = siwx91x_wuc_disable,
	.triggered = siwx91x_wuc_triggered,
	.clear = siwx91x_wuc_clear,
};

#define SIWX91X_WUC_INIT(inst)                                                                     \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                          \
			      CONFIG_WUC_INIT_PRIORITY, &siwx91x_wuc_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_WUC_INIT)
