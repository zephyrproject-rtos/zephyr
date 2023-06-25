/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>

/* Used for driver binding */
#define DT_DRV_COMPAT nxp_imx_ccm_rev3

extern struct imx_ccm_data mcux_ccm_data;
extern struct imx_ccm_config mcux_ccm_config;

static int mcux_ccm_on_off(const struct device *dev,
		clock_control_subsys_t sys, bool on)
{
	uint32_t clock_name;
	const struct imx_ccm_config *cfg;

	clock_name = (uintptr_t)sys;
	cfg = dev->config;

	/* validate clock_name */
	if (clock_name >= cfg->clock_config->clock_num)
		return -EINVAL;

	return imx_ccm_clock_on_off(dev, &cfg->clock_config->clocks[clock_name], on);
}

static int mcux_ccm_on(const struct device *dev, clock_control_subsys_t sys)
{
	return mcux_ccm_on_off(dev, sys, true);
}

static int mcux_ccm_off(const struct device *dev, clock_control_subsys_t sys)
{
	return mcux_ccm_on_off(dev, sys, false);
}

static int mcux_ccm_get_rate(const struct device *dev,
		clock_control_subsys_t sys, uint32_t *rate)
{
	uint32_t clock_name, returned_rate;
	const struct imx_ccm_config *cfg;

	clock_name = (uintptr_t)sys;
	cfg = dev->config;

	/* validate clock_name */
	if (clock_name >= cfg->clock_config->clock_num)
		return -EINVAL;

	returned_rate = imx_ccm_clock_get_rate(dev, &cfg->clock_config->clocks[clock_name]);

	if (returned_rate > 0) {
		*rate = returned_rate;
		return 0;
	} else {
		return returned_rate;
	}
}

static int mcux_ccm_set_rate(const struct device *dev,
		clock_control_subsys_t sys, clock_control_subsys_rate_t rate)
{
	uint32_t clock_name, requested_rate;
	const struct imx_ccm_config *cfg;

	clock_name = (uintptr_t)sys;
	requested_rate = (uintptr_t)rate;
	cfg = dev->config;

	/* validate clock_name */
	if (clock_name >= cfg->clock_config->clock_num)
		return -EINVAL;

	return imx_ccm_clock_set_rate(dev, &cfg->clock_config->clocks[clock_name], requested_rate);
}

static int mcux_ccm_init(const struct device *dev)
{
	return imx_ccm_init(dev);
}

static const struct clock_control_driver_api mcux_ccm_driver_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_rate,
	.set_rate = mcux_ccm_set_rate,
};

/* there's only one CCM per SoC */
DEVICE_DT_INST_DEFINE(0,
		&mcux_ccm_init,
		NULL,
		&mcux_ccm_data, &mcux_ccm_config,
		PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		&mcux_ccm_driver_api);
