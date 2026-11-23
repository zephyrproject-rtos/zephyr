/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_clock_ckout

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>

#include <gd32_rcu.h>

struct gd32_ckout_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t source;
	uint32_t divider;
};

static int gd32_ckout_init(const struct device *dev)
{
	const struct gd32_ckout_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	rcu_ckout0_config(config->source, config->divider);

	return 0;
}

#define GD32_CKOUT_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct gd32_ckout_config gd32_ckout_config_##inst = {                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.source = DT_INST_CLOCKS_CELL_BY_NAME(inst, clksel, id),                           \
		.divider = DT_INST_PROP(inst, prescaler),                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gd32_ckout_init, NULL, NULL, &gd32_ckout_config_##inst,        \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(GD32_CKOUT_INIT)
