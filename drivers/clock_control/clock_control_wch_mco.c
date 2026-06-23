/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

struct wch_mco_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock;
	uint32_t src;
	uint32_t clock_id;
};

static int wch_mco_init(const struct device *dev)
{
	const struct wch_mco_config *config = dev->config;
	int ret;

	ret = clock_control_configure(config->clock, (clock_control_subsys_t *)config->clock_id,
				      (void *)config->src);
	if (ret < 0) {
		return ret;
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

#define WCH_MCO_DEFINE(node_id)                                                                    \
	PINCTRL_DT_DEFINE(node_id);                                                                \
	static const struct wch_mco_config wch_mco_config_##node_id = {                            \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(node_id),                                        \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR(node_id)),                                   \
		.src = DT_ENUM_IDX(node_id, source) + 4,                                           \
		.clock_id = DT_CLOCKS_CELL(node_id, id),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, wch_mco_init, NULL, NULL, &wch_mco_config_##node_id,             \
			 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_FOREACH_STATUS_OKAY(wch_ch32v20x_30x_clock_mco, WCH_MCO_DEFINE);
