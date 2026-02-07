/*
 * Copyright (c) 2026 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT elan_em32_apb

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(em32_apb, CONFIG_LOG_DEFAULT_LEVEL);

struct elan_em32_apb_clock_control_config {
	const struct device *clock_device;
	uint32_t parent_gate_id;
};

static int elan_em32_apb_clock_control_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct elan_em32_apb_clock_control_config *config = dev->config;

	/* Ensure parent clock reference is enabled (no-op for EM32_GATE_NONE). */
	(void)clock_control_on(config->clock_device, UINT_TO_POINTER(config->parent_gate_id));

	/* Then enable requested APB gate (forwarded to AHB unified controller). */
	return clock_control_on(config->clock_device, sys);
}

static int elan_em32_apb_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct elan_em32_apb_clock_control_config *config = dev->config;

	/* Do not disable parent here; it may be shared by other consumers. */
	return clock_control_off(config->clock_device, sys);
}

static int elan_em32_apb_clock_control_get_rate(const struct device *dev,
						clock_control_subsys_t sys, uint32_t *rate)
{
	const struct elan_em32_apb_clock_control_config *config = dev->config;
	int ret = 0;
	uint32_t ahb_clk_rate = 0;
	uint32_t apb_clk_rate = 0;

	ARG_UNUSED(sys);

	/* Get AHB Clock Rate */
	ret = clock_control_get_rate(config->clock_device, CLOCK_CONTROL_SUBSYS_ALL, &ahb_clk_rate);
	if (ret) {
		LOG_ERR("Fail to Get AHB Clock Rate, err=%d.", ret);
		return ret;
	}

	apb_clk_rate = ahb_clk_rate / 2; /* Fix value of clock divider (2) */
	*rate = apb_clk_rate;

	return ret;
}

static DEVICE_API(clock_control, elan_em32_apb_clock_control_api) = {
	.on = elan_em32_apb_clock_control_on,
	.off = elan_em32_apb_clock_control_off,
	.get_rate = elan_em32_apb_clock_control_get_rate,
};

static int elan_em32_apb_clock_control_init(const struct device *dev)
{
	const struct elan_em32_apb_clock_control_config *config = dev->config;

	if (!device_is_ready(config->clock_device)) {
		LOG_ERR("Clock source not ready!");
		return -ENODEV;
	}

	LOG_DBG("Initialized.");

	return 0;
}

#define EM32_APB_INST_INIT(inst)                                                                   \
	static const struct elan_em32_apb_clock_control_config em32_apb_config_##inst = {          \
		.clock_device = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                          \
		.parent_gate_id = DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, gate_id),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, elan_em32_apb_clock_control_init, NULL, NULL,                  \
			      &em32_apb_config_##inst, PRE_KERNEL_1,                               \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                                  \
			      &elan_em32_apb_clock_control_api)

DT_INST_FOREACH_STATUS_OKAY(EM32_APB_INST_INIT)
