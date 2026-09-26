/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#define CLOCK_WCH_MCO_ENABLE 4

struct clock_wch_mco_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	uint32_t clock_id;
	uint32_t src;
};

static int clock_wch_mco_init(const struct device *dev)
{
	const struct clock_wch_mco_config *const config = dev->config;
	int ret;

	ret = clock_control_configure(config->clock_dev, (clock_control_subsys_t *)config->clock_id,
				      (void *)config->src);
	if (ret < 0) {
		return ret;
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

#define CLOCK_WCH_MCO_DEFINE(node)                                                                 \
	PINCTRL_DT_DEFINE(node);                                                                   \
	static const struct clock_wch_mco_config clock_wch_mco_config_##node = {                   \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(node),                                           \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(node)),                                  \
		.clock_id = DT_CLOCKS_CELL(node, id),                                              \
		.src = DT_ENUM_IDX(node, source) + CLOCK_WCH_MCO_ENABLE,                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, clock_wch_mco_init, NULL, NULL, &clock_wch_mco_config_##node,       \
			 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_FOREACH_STATUS_OKAY(wch_ch32v20x_30x_clock_mco, CLOCK_WCH_MCO_DEFINE);
