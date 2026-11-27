/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_wkpu_wuc

#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/init.h>

#include "../interrupt_controller/intc_wkpu_nxp_s32_priv.h"

struct wuc_nxp_s32_wkpu_config {
	const struct device *parent;
};

static int wuc_nxp_s32_wkpu_enable(const struct device *dev, uint32_t source)
{
	const struct wuc_nxp_s32_wkpu_config *config = dev->config;

	return wkpu_nxp_s32_wuc_enable(config->parent, source, true);
}

static int wuc_nxp_s32_wkpu_disable(const struct device *dev, uint32_t source)
{
	const struct wuc_nxp_s32_wkpu_config *config = dev->config;

	return wkpu_nxp_s32_wuc_enable(config->parent, source, false);
}

static int wuc_nxp_s32_wkpu_triggered(const struct device *dev, uint32_t source)
{
	const struct wuc_nxp_s32_wkpu_config *config = dev->config;

	return wkpu_nxp_s32_wuc_triggered(config->parent, source);
}

static int wuc_nxp_s32_wkpu_clear(const struct device *dev, uint32_t source)
{
	const struct wuc_nxp_s32_wkpu_config *config = dev->config;

	return wkpu_nxp_s32_wuc_clear(config->parent, source);
}

static int wuc_nxp_s32_wkpu_init(const struct device *dev)
{
	const struct wuc_nxp_s32_wkpu_config *config = dev->config;

	return device_is_ready(config->parent) ? 0 : -ENODEV;
}

static DEVICE_API(wuc, wuc_nxp_s32_wkpu_api) = {
	.enable = wuc_nxp_s32_wkpu_enable,
	.disable = wuc_nxp_s32_wkpu_disable,
	.triggered = wuc_nxp_s32_wkpu_triggered,
	.clear = wuc_nxp_s32_wkpu_clear,
};

BUILD_ASSERT(CONFIG_WUC_INIT_PRIORITY > CONFIG_INTC_INIT_PRIORITY,
	     "WKPU WUC must initialize after its interrupt controller parent");

#define WUC_NXP_S32_WKPU_DEFINE(inst)							\
	static const struct wuc_nxp_s32_wkpu_config wuc_nxp_s32_wkpu_config_##inst = {	\
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),			\
	};										\
	DEVICE_DT_INST_DEFINE(inst, wuc_nxp_s32_wkpu_init, NULL, NULL,			\
			      &wuc_nxp_s32_wkpu_config_##inst, PRE_KERNEL_2,		\
			      CONFIG_WUC_INIT_PRIORITY, &wuc_nxp_s32_wkpu_api);

DT_INST_FOREACH_STATUS_OKAY(WUC_NXP_S32_WKPU_DEFINE)
