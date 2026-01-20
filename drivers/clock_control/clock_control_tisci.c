/*
 * Copyright (c) 2025, Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_k2g_sci_clk

#include <zephyr/device.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ti_k2g_sci_clk, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static const struct device *dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));

static int tisci_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	struct tisci_clock_config *req = (struct tisci_clock_config *)sys;
	uint64_t temp_rate;
	int ret;

	ret = tisci_cmd_clk_get_freq(dmsc, req->dev_id, req->clk_id, &temp_rate);
	if (ret) {
		LOG_ERR("Failed to get clock freq: dev_id=%u clk_id=%u err=%d", req->dev_id,
			req->clk_id, ret);
		return ret;
	}

	*rate = (uint32_t)temp_rate;

	return 0;
}

static int tisci_set_rate(const struct device *dev, void *sys, void *rate)
{
	struct tisci_clock_config *req = (struct tisci_clock_config *)sys;
	uint64_t freq = *((uint64_t *)rate);
	int ret;

	ret = tisci_cmd_clk_set_freq(dmsc, req->dev_id, req->clk_id, freq, freq, freq);
	if (ret) {
		LOG_ERR("Failed to set clock freq: dev_id=%u clk_id=%u freq=%llu err=%d",
			req->dev_id, req->clk_id, freq, ret);
	}

	return ret;
}

static enum clock_control_status tisci_get_status(const struct device *dev,
						  clock_control_subsys_t sys)
{
	struct tisci_clock_config *req = (struct tisci_clock_config *)sys;
	bool req_state = true;
	bool curr_state = true;
	int ret;

	ret = tisci_cmd_clk_is_on(dmsc, req->clk_id, req->dev_id, &req_state, &curr_state);
	if (ret) {
		LOG_ERR("Failed to get clock ON status: dev_id=%u clk_id=%u err=%d", req->dev_id,
			req->clk_id, ret);
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
	if (curr_state) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	if (req_state) {
		return CLOCK_CONTROL_STATUS_STARTING;
	}

	curr_state = true;

	ret = tisci_cmd_clk_is_off(dmsc, req->clk_id, req->dev_id, NULL, &curr_state);
	if (ret) {
		LOG_ERR("Failed to get clock OFF status: dev_id=%u clk_id=%u err=%d", req->dev_id,
			req->clk_id, ret);
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (curr_state) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	return CLOCK_CONTROL_STATUS_UNKNOWN;
}

static DEVICE_API(clock_control, tisci_clock_driver_api) = {
	.get_rate = tisci_get_rate,
	.set_rate = tisci_set_rate,
	.get_status = tisci_get_status
};

#define TI_K2G_SCI_CLK_INIT(_n)                                                                    \
	DEVICE_DT_INST_DEFINE(_n, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                            \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &tisci_clock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TI_K2G_SCI_CLK_INIT)
