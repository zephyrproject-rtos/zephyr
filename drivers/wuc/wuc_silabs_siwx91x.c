/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_wuc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/silabs-siwx91x-wuc.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include "rsi_system_config.h"
#include <rsi_power_save.h>
#include <sl_si91x_power_manager.h>

struct wakeup_source_config {
	uint32_t wakeup_src_mask;
	uint32_t mcu_fsm_clear_mask;
};

static const struct wakeup_source_config wakeup_sources[] = {
	[SIWX91X_WUC_RTC_ALARM] = {
		.wakeup_src_mask = SL_SI91X_POWER_MANAGER_ALARM_WAKEUP,
		.mcu_fsm_clear_mask = RTC_ALARM_BASED_WAKEUP_STATUS_CLEAR,
	},
	[SIWX91X_WUC_GPIO] = {
		.wakeup_src_mask = SL_SI91X_POWER_MANAGER_GPIO_WAKEUP,
	},
};

struct siwx91x_wuc_config {
	mem_addr_t mcu_fsm;
	const struct wakeup_source_config *sources;
	size_t source_count;
};

struct siwx91x_wuc_data {
	struct k_spinlock lock;
};

static const struct wakeup_source_config *
siwx91x_wuc_find_source(const struct siwx91x_wuc_config *config, uint32_t id)
{
	if (id >= config->source_count) {
		return NULL;
	}

	return &config->sources[id];
}

static int siwx91x_wuc_source_to_mask(const struct device *dev, uint32_t id, uint32_t *mask)
{
	const struct siwx91x_wuc_config *config = dev->config;
	const struct wakeup_source_config *current_source = siwx91x_wuc_find_source(config, id);

	if (current_source == NULL) {
		return -EINVAL;
	}

	*mask = current_source->wakeup_src_mask;
	return 0;
}

static int siwx91x_wuc_enable(const struct device *dev, uint32_t id)
{
	struct siwx91x_wuc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;
	int ret;

	ret = siwx91x_wuc_source_to_mask(dev, id, &mask);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);
	RSI_PS_SetWkpSources(mask);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int siwx91x_wuc_disable(const struct device *dev, uint32_t id)
{
	struct siwx91x_wuc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;
	int ret;

	ret = siwx91x_wuc_source_to_mask(dev, id, &mask);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);
	RSI_PS_ClrWkpSources(mask);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int siwx91x_wuc_clear(const struct device *dev, uint32_t id)
{
	const struct siwx91x_wuc_config *config = dev->config;
	const struct wakeup_source_config *current_source = siwx91x_wuc_find_source(config, id);

	if (current_source == NULL) {
		return -EINVAL;
	}

	if (current_source->mcu_fsm_clear_mask == 0U) {
		return -ENOTSUP;
	}

	sys_write32(current_source->mcu_fsm_clear_mask, config->mcu_fsm);
	return 0;
}

static DEVICE_API(wuc, siwx91x_wuc_api) = {
	.enable = siwx91x_wuc_enable,
	.disable = siwx91x_wuc_disable,
	.clear = siwx91x_wuc_clear,
};

#define SIWX91X_WUC_INIT(inst)                                                                     \
	static struct siwx91x_wuc_data siwx91x_wuc_data_##inst;                                    \
	static const struct siwx91x_wuc_config siwx91x_wuc_config_##inst = {                       \
		.mcu_fsm = DT_INST_REG_ADDR_BY_NAME(inst, mcu_fsm),                                \
		.sources = wakeup_sources,                                                         \
		.source_count = ARRAY_SIZE(wakeup_sources),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &siwx91x_wuc_data_##inst,                          \
			      &siwx91x_wuc_config_##inst, PRE_KERNEL_1, CONFIG_WUC_INIT_PRIORITY,  \
			      &siwx91x_wuc_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_WUC_INIT)
