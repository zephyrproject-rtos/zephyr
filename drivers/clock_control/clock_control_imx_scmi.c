/*
 * Copyright 2024 NXP
 *
 * Authors:
 *  Hou Zhiqiang <Zhiqiang.Hou@nxp.com>
 *  Yangbo Lu <yangbo.lu@nxp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_clk_scmi
#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include "clock_soc.h"
#include "scmi_clock.h"

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);


static int imx_clk_scmi_enable_clk(uint32_t clk_id, bool enable)
{
	return SCMI_ClockConfigSet(SM_A2P, clk_id, SCMI_CLOCK_CONFIG_SET_ENABLE(enable), 0);
}

static int imx_clk_scmi_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (size_t)sub_system;

	return imx_clk_scmi_enable_clk(clk_id, true);
}

static int imx_clk_scmi_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (size_t)sub_system;

	return imx_clk_scmi_enable_clk(clk_id, false);
}

static int imx_clk_scmi_get_subsys_rate(const struct device *dev, clock_control_subsys_t sub_system,
		uint32_t *rate)
{
	uint32_t clk_id = (size_t)sub_system;
	uint32_t pclk_id;
	scmi_clock_rate_t clk_rate = {0, 0};

	if (clk_id >= SCMI_CLK_SRC_NUM) {
		if (SCMI_ClockParentGet(SM_A2P, clk_id, &pclk_id))
			return -EIO;
	}

	if (SCMI_ClockRateGet(SM_A2P, clk_id, &clk_rate))
		return -EIO;

	*rate = ((uint64_t)(clk_rate.upper) << 32) | clk_rate.lower;

	return 0;
}

static int imx_clk_scmi_set_subsys_rate(const struct device *dev, clock_control_subsys_t sub_system,
		clock_control_subsys_rate_t rate)
{
	uint32_t attributes = SCMI_CLOCK_CONFIG_SET_ENABLE(true);
	uint32_t flags = SCMI_CLOCK_RATE_FLAGS_ROUND(SCMI_CLOCK_ROUND_AUTO);
	uint32_t clk_id = (size_t)sub_system;
	uint64_t set_rate = (uintptr_t)rate;
	scmi_clock_rate_t clock_rate = {0, 0};

	clock_rate.lower = set_rate & 0xffffffff;
	clock_rate.upper = (set_rate >> 32) & 0xffffffff;

	if (SCMI_ClockRateSet(SM_A2P, clk_id, flags, clock_rate))
		return -EIO;

	if (clk_id < SCMI_CLK_SRC_NUM)
		return 0;

	return SCMI_ClockConfigSet(SM_A2P, clk_id, attributes, 0);
}

static const struct clock_control_driver_api imx_clk_scmi_driver_api = {
	.on = imx_clk_scmi_on,
	.off = imx_clk_scmi_off,
	.get_rate = imx_clk_scmi_get_subsys_rate,
	.set_rate = imx_clk_scmi_set_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &imx_clk_scmi_driver_api);
